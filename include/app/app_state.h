#pragma once
#include <Arduino.h>

struct AppReadings {
  float tC = NAN;
  float rh = NAN;
  bool dhtOk = false;
  uint16_t mq7Raw = 0;
  float mq7Ratio = NAN;
  float mq7Ppm = NAN;
  float mq7R0 = NAN;
  bool mq7Ok = false;
  bool mq7Calibrated = false;
  bool mq7WarmupDone = false;
  uint8_t mq7Level = 0; // 0=UNKNOWN, 1=OK, 2=WARN, 3=DANGER
};
