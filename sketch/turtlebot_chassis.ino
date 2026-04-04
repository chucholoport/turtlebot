/**
 * @file turtlebot_chassis.ino
 * @brief Differential drive chassis controller using Arduino RouterBridge.
 *
 * @details
 * This firmware exposes an RPC interface to receive velocity commands
 * from a high-level system (ROS 2 via bridge).
 *
 * Linear (vx) and angular (wz) velocities are used to compute left
 * and right wheel velocities using differential drive kinematics.
 */

#include "Arduino_RouterBridge.h"

/**
 * ------------------------------------------------------------
 * @brief Control parameters and state variables.
 * ------------------------------------------------------------
 */

/** @brief Maximum linear velocity corresponding to full PWM. */
static const float MAX_LINEAR = 0.5f;

/** @brief Maximum angular velocity. */
static const float MAX_ANGULAR = 2.0f;

/** @brief Distance between wheels [m]. */
static const float WHEEL_BASE = 0.20f;

/** @brief Maximum PWM value for motor driver. */
static const int MAX_PWM = 255;

/** @brief Global linear velocity command. */
static volatile float g_linear = 0.0f;

/** @brief Global angular velocity command. */
static volatile float g_angular = 0.0f;

/**
 * ------------------------------------------------------------
 * @brief Motor driver pin definitions.
 * ------------------------------------------------------------
 */

/** @brief LEFT motor EN: PWM speed control. */
static const int PIN_L_EN  = 5;

/** @brief LEFT motor IN1: Direction control. */
static const int PIN_L_IN1 = 6;

/** @brief LEFT motor IN2: Direction control. */
static const int PIN_L_IN2 = 7;

/** @brief RIGHT motor EN: PWM speed control. */
static const int PIN_R_EN  = 9;

/** @brief RIGHT motor IN1: Direction control. */
static const int PIN_R_IN1 = 10;

/** @brief RIGHT motor IN2: Direction control. */
static const int PIN_R_IN2 = 11;


/**
 * ------------------------------------------------------------
 * @brief Encoder parameters and state variables.
 * ------------------------------------------------------------
 */

/** @brief Encoder pulses per revolution. */
static const int ENCODER_PPR = 20;

/** @brief Wheel radius [m]. */
static const float WHEEL_RADIUS = 0.03f;

/** @brief Left encoder pin (INT0). */
static const int PIN_ENC_L_A = 2;
static const int PIN_ENC_L_B = 4;

/** @brief Right encoder pin (INT1). */
static const int PIN_ENC_R_A = 3;
static const int PIN_ENC_R_B = 12;

/** @brief Encoder tick counters. */
static volatile long g_enc_left = 0;
static volatile long g_enc_right = 0;

/**
 * ------------------------------------------------------------
 * @brief Function prototypes.
 * ------------------------------------------------------------
 */

static inline int linear_to_pwm(float v);
static void setMotor(int pwm, int in1, int in2, int en);

void set_cmd_vel(float vx, float wz);

void isr_enc_left(void);
void isr_enc_right(void);

void send_encoder_update(long left, long right, float vel_left, float vel_right);

static float ticks_to_velocity(long ticks, float dt);



void setup()
{
    pinMode(PIN_L_EN, OUTPUT);
    pinMode(PIN_L_IN1, OUTPUT);
    pinMode(PIN_L_IN2, OUTPUT);

    pinMode(PIN_R_EN, OUTPUT);
    pinMode(PIN_R_IN1, OUTPUT);
    pinMode(PIN_R_IN2, OUTPUT);

    pinMode(PIN_ENC_L_A, INPUT_PULLUP);
    pinMode(PIN_ENC_L_B, INPUT_PULLUP);

    pinMode(PIN_ENC_R_A, INPUT_PULLUP);
    pinMode(PIN_ENC_R_B, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(PIN_ENC_L_A), isr_enc_left, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_R_A), isr_enc_right, CHANGE);

    Bridge.begin();

    /**
     * @note
     * provide_safe ensures execution in main loop context
     */
    Bridge.provide_safe("set_cmd_vel", set_cmd_vel);
}

void loop()
{
    /* Time base */
    static unsigned long prev_time = 0;
    unsigned long now = millis();

    if (prev_time == 0)
    {
        prev_time = now;
        return;
    }

    float dt = (now - prev_time) / 1000.0f;
    prev_time = now;

    /* Copy shared state */
    float v = g_linear;
    float w = g_angular;

    /* Kinematics */
    float v_left  = v - (w * WHEEL_BASE * 0.5f);
    float v_right = v + (w * WHEEL_BASE * 0.5f);

    int pwm_left  = linear_to_pwm(v_left);
    int pwm_right = linear_to_pwm(v_right);

    /* Encoder read (atomic) */
    long enc_left;
    long enc_right;

    noInterrupts();
    enc_left  = g_enc_left;
    enc_right = g_enc_right;
    interrupts();

    /* Delta ticks */
    static long prev_enc_left = 0;
    static long prev_enc_right = 0;

    long delta_left  = enc_left  - prev_enc_left;
    long delta_right = enc_right - prev_enc_right;

    prev_enc_left  = enc_left;
    prev_enc_right = enc_right;

    /* Velocity estimation */
    float vel_left  = ticks_to_velocity(delta_left, dt);
    float vel_right = ticks_to_velocity(delta_right, dt);

    /* Publish (throttled) */
    static unsigned long prev_pub = 0;

    if ((now - prev_pub) >= 50) /* 20 Hz */
    {
        send_encoder_update(enc_left, enc_right, vel_left, vel_right);
        prev_pub = now;
    }

    /* Actuation */
    setMotor(pwm_left,  PIN_L_IN1, PIN_L_IN2, PIN_L_EN);
    setMotor(pwm_right, PIN_R_IN1, PIN_R_IN2, PIN_R_EN);

    delay(10);
}



/**
 * -------------------------------------------------------------
 * @brief Core logic for motor control and RPC handling.
 * -------------------------------------------------------------
 */

/**
 * @brief Convert linear velocity [m/s] to PWM command.
 */
static inline int linear_to_pwm(float v)
{
    float norm = (MAX_LINEAR != 0.0f) ? (v / MAX_LINEAR) : 0.0f;

    if (norm > 1.0f) norm = 1.0f;
    if (norm < -1.0f) norm = -1.0f;

    int pwm = (int)(norm * MAX_PWM + (norm >= 0.0f ? 0.5f : -0.5f));

    if (pwm > MAX_PWM) pwm = MAX_PWM;
    else if (pwm < -MAX_PWM) pwm = -MAX_PWM;

    return pwm;
}

/**
 * @brief Apply PWM signal to motor driver.
 */
static void setMotor(int pwm, int in1, int in2, int en)
{
    /* Defensive saturation */
    if (pwm > MAX_PWM) pwm = MAX_PWM;
    else if (pwm < -MAX_PWM) pwm = -MAX_PWM;

    if (pwm > 0)
    {
        digitalWrite(in1, HIGH);
        digitalWrite(in2, LOW);
        analogWrite(en, pwm);
    }
    else if (pwm < 0)
    {
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
        analogWrite(en, -pwm);
    }
    else
    {
        digitalWrite(in1, LOW);
        digitalWrite(in2, LOW);
        analogWrite(en, 0);
    }
}

/**
 * @brief Left encoder ISR.
 */
void isr_enc_left(void)
{
    int b = digitalRead(PIN_ENC_L_B);

    if (b == HIGH) g_enc_left++;
    else g_enc_left--;
}

/**
 * @brief Right encoder ISR.
 */
void isr_enc_right(void)
{
    int b = digitalRead(PIN_ENC_R_B);

    if (b == HIGH) g_enc_right++;
    else g_enc_right--;
}

/**
 * @brief Convert encoder ticks to linear velocity [m/s]
 */
static float ticks_to_velocity(long ticks, float dt)
{
    if (dt <= 0.0f) return 0.0f;

    float rev = (float)ticks / ENCODER_PPR;
    float dist = rev * 2.0f * 3.1415926f * WHEEL_RADIUS;
    return dist / dt;
}

/**
 * @brief Send encoder data via RPC (non-blocking).
 *
 * @param left       Left encoder ticks
 * @param right      Right encoder ticks
 * @param vel_left   Left wheel velocity [m/s]
 * @param vel_right  Right wheel velocity [m/s]
 */
void send_encoder_update(long left, long right, float vel_left, float vel_right)
{
    Bridge.notify("encoder_update", left, right, vel_left, vel_right);
}

/**
 * @brief RPC handler for velocity command.
 */
void set_cmd_vel(float vx, float wz)
{
    g_linear  = vx;
    g_angular = wz;
}