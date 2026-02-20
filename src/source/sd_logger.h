#pragma once

#include <Arduino.h>
#include "app/app_state.h"

class SdLogger {
public:
  bool begin();
  void update(uint32_t nowMs, const AppReadings& readings, uint32_t unixTs = 0);
  bool appendNow(const AppReadings& readings, uint32_t unixTs = 0);

  bool isReady() const { return ready_; }

private:
  bool ensureFileHasHeader_();

  bool ready_ = false;
  uint32_t nextWriteAtMs_ = 0;
};
