#pragma once
#include <Arduino.h>

struct DhtReading {
  float tC;
  float rh;
  bool ok;
};

class DhtSensor {
public:
  void begin();
  void update(uint32_t nowMs);
  DhtReading get() const { return last_; }

private:
  uint32_t nextRead_ = 0;
  DhtReading last_{NAN, NAN, false};
};