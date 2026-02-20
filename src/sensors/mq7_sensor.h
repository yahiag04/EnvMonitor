#pragma once
#include <Arduino.h>
#include "sensors/mq7_types.h"
#include "storage/r0_store.h"

class Mq7Sensor {
public:
  void begin();
  void update(uint32_t nowMs);
  Mq7Reading get() const { return last_; }

  // Calibrazione: chiama quando sei in aria pulita (dopo warmup)
  // Salva R0 persistente
  bool calibrateNow(uint8_t samples = 20);
  void resetCalibration();
  bool isCalibrated() const { return calibrated_; }
  bool isWarmupDone(uint32_t nowMs) const { return nowMs >= warmupUntilMs_; }

private:
  R0Store store_;
  float r0_ = NAN;
  bool calibrated_ = false;
  uint32_t warmupUntilMs_ = 0;
  uint32_t nextSampleAtMs_ = 0;

  Mq7Reading last_;

  uint16_t readAvgRaw_() const;

  // conversioni
  float rawToVnode_(uint16_t raw) const;
  float computeRs_(float vRl) const;
  float ratioToPpm_(float ratio) const;
};
