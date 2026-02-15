#pragma once
#include <Arduino.h>
#include "app/app_state.h"

namespace net {

struct TelemetryPayload {
  AppReadings readings;
  uint32_t ts; // epoch seconds (0 se non disponibile)
};

bool postTelemetry(const TelemetryPayload& p);

} // namespace net