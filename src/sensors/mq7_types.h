#pragma once
#include <Arduino.h>

struct Mq7Reading {
  uint16_t raw = 0;     // ADC raw 0..4095 (nodo)
  float vNode = NAN;    // volt al nodo (stima)
  float vRl = NAN;      // VRL prima del partitore (stimata)
  float rs = NAN;       // sensor resistance (ohm)
  float r0 = NAN;       // baseline (ohm)
  bool calibrated = false;
  bool warmupDone = false;
  float ratio = NAN;    // Rs/R0
  float ppm = NAN;      // stima CO ppm
  bool ok = false;
};
