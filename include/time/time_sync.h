#pragma once
#include <Arduino.h>

namespace timeutil {
  void beginNtp();              // chiama configTime
  bool isTimeValid();           // true quando time() è “sensato”
  uint32_t unixTime();          // epoch seconds (0 se non valido)
}