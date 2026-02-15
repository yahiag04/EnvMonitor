#pragma once
#include <Arduino.h>
#include "sensors/mq7_types.h"
#include "drivers/heater_controller.h"
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
  enum class Phase { HEAT_HIGH, HEAT_LOW };

  Phase phase_ = Phase::HEAT_HIGH;
  uint32_t phaseUntil_ = 0;

  HeaterController heater_;
  R0Store store_;
  float r0_ = NAN;
  bool calibrated_ = false;
  uint32_t warmupUntilMs_ = 0;

  Mq7Reading last_;

  uint16_t readAvgRaw_() const;
  void switchPhase_(Phase p, uint32_t nowMs);

  // conversioni
  float rawToVnode_(uint16_t raw) const;
  float computeRs_(float vRl) const;
  float ratioToPpm_(float ratio) const;
};
