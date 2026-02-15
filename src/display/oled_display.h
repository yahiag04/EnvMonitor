#pragma once
#include <Arduino.h>

class OledDisplay {
public:
  void begin();
  void update(float tC, float rh, float coPpm, float ratio, bool mqOk, bool calibrated, bool warmupDone, uint8_t mq7Level);

private:
  uint32_t nextDraw_ = 0;
};
