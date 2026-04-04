/**
 * @file check_wheel.ino
 * @brief Single wheel driver using Arduino RouterBridge.
 *
 * @details
 * This firmware exposes an RPC interface to receive velocity commands
 * from a high-level system (ROS 2 via bridge).
 *
 * Only linear velocity (vx) is used. Angular velocity is ignored.
 */

#include "Arduino_RouterBridge.h"

/**
 * ------------------------------------------------------------
 * @brief Control parameters and state variables.
 * ------------------------------------------------------------
 */

/** @brief Maximum linear velocity corresponding to full PWM. */
static const float MAX_LINEAR = 0.5f;

/** @brief Maximum PWM value for motor driver. */
static const int MAX_PWM = 255;

/** @brief Global linear velocity command. */
static volatile float g_linear = 0.0f;

/**
 * ------------------------------------------------------------
 * @brief Motor driver pin definitions.
 * @note: Adjust pin numbers as needed for your hardware setup.
 * ------------------------------------------------------------
 */

/** @brief IN1: Direction control. */
static const int PIN_IN1 = 5;

/** @brief IN2: Direction control. */
static const int PIN_IN2 = 6;

/** @brief EN: PWM speed control. */
static const int PIN_EN  = 9;

/**
 * ------------------------------------------------------------
 * @brief Function prototypes.
 * ------------------------------------------------------------
 */

static inline int linear_to_pwm(float v);

void setMotor(int pwm);

void set_cmd_vel(float vx, float wz);


void setup()
{
    pinMode(PIN_IN1, OUTPUT);
    pinMode(PIN_IN2, OUTPUT);
    pinMode(PIN_EN, OUTPUT);

    Bridge.begin();

    /**
     * @note
     * provide_safe ensures execution in main loop context
     */
    Bridge.provide_safe("set_cmd_vel", set_cmd_vel);
}


void loop()
{
    int pwm = linear_to_pwm(g_linear);

    setMotor(pwm);

    delay(10);
}


/**
 * -------------------------------------------------------------
 * @brief Core logic for motor control and RPC handling.
 * -------------------------------------------------------------
 */

/**
 * @brief Convert linear velocity [m/s] to PWM command.
 *
 * @param v Linear velocity input [m/s]
 * @return PWM value in range [-MAX_PWM, MAX_PWM]
 */
static inline int linear_to_pwm(float v)
{
    /* Normalize */
    float norm = (MAX_LINEAR != 0.0f) ? (v / MAX_LINEAR) : 0.0f;

    /* Clamp */
    if (norm > 1.0f) norm = 1.0f;
    if (norm < -1.0f) norm = -1.0f;

    /* Convert to PWM with rounding */
    int pwm = (int)(norm * MAX_PWM + (norm >= 0.0f ? 0.5f : -0.5f));

    /* Saturate to valid range */
    if (pwm > MAX_PWM) pwm = MAX_PWM;
    else if (pwm < -MAX_PWM) pwm = -MAX_PWM;

    return pwm;
}

/**
 * @brief Apply PWM signal to motor driver.
 */
void setMotor(int pwm)
{
    if (pwm > 0)
    {
        digitalWrite(PIN_IN1, HIGH);
        digitalWrite(PIN_IN2, LOW);
        analogWrite(PIN_EN, pwm);
    }
    else if (pwm < 0)
    {
        digitalWrite(PIN_IN1, LOW);
        digitalWrite(PIN_IN2, HIGH);
        analogWrite(PIN_EN, -pwm);
    }
    else
    {
        digitalWrite(PIN_IN1, LOW);
        digitalWrite(PIN_IN2, LOW);
        analogWrite(PIN_EN, 0);
    }
}

/**
 * @brief RPC handler for velocity command.
 *
 * @param vx Linear velocity [m/s]
 * @param wz Angular velocity [rad/s] (unused)
 */
void set_cmd_vel(float vx, float wz)
{
    (void)wz;
    g_linear = vx;
}