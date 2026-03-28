#include "Arduino_RouterBridge.h"

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Bridge.begin();
  // We use provide_safe to ensure the hardware call runs in the main loop context
  Bridge.provide_safe("set_led_state", set_led_state);
}

void loop() {
}

void set_led_state(bool state) {
  digitalWrite(LED_BUILTIN, state ? LOW : HIGH);
}