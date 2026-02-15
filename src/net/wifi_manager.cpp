#include "net/wifi_manager.h"
#include <WiFi.h>
#include "config.h"

namespace net {

static uint32_t nextTryMs = 0;

void wifiBegin() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
}

bool wifiIsConnected() {
  return WiFi.status() == WL_CONNECTED;
}

String wifiIp() {
  return WiFi.localIP().toString();
}

void wifiEnsureConnected(uint32_t nowMs) {
  if (wifiIsConnected()) return;

  if (nowMs < nextTryMs) return;
  nextTryMs = nowMs + 3000; // ritenta ogni 3s

  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

} // namespace net