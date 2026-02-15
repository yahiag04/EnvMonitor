#pragma once
#include <Arduino.h>

class HeaterController {
public:
  void begin(int pinHeater);
  void setHigh();                // 5V (ON)
  void setLowEquivalent1V4();    // ~1.4V equivalent via PWM
  void off();

private:
  int pin_ = -1;
  int pwmChannel_ = 0;
  int pwmFreq_ = 2000;     // heater: freq non critica (inerzia termica)
  int pwmResBits_ = 8;     // 0..255
};