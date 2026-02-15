#pragma once
#include <Arduino.h>

namespace net {
  void wifiBegin();
  void wifiEnsureConnected(uint32_t nowMs);
  bool wifiIsConnected();
  String wifiIp();
}