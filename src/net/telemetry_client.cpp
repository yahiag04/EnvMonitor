#include "net/telemetry_client.h"
#include "net/wifi_manager.h"
#include "config.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>

namespace net {

bool postTelemetry(const TelemetryPayload& p) {
  if (!wifiIsConnected()) return false;

  HTTPClient http;
  http.begin(TELEMETRY_URL);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + DEVICE_TOKEN);

  StaticJsonDocument<512> doc;
  doc["deviceId"] = DEVICE_ID;
  doc["ts"] = p.ts;
  doc["t"] = p.readings.tC;
  doc["rh"] = p.readings.rh;
  doc["dhtOk"] = p.readings.dhtOk;
  doc["mq7Raw"] = p.readings.mq7Raw;
  doc["mq7Ratio"] = p.readings.mq7Ratio;
  doc["mq7Ppm"] = p.readings.mq7Ppm;
  doc["mq7R0"] = p.readings.mq7R0;
  doc["mq7Ok"] = p.readings.mq7Ok;
  doc["mq7Calibrated"] = p.readings.mq7Calibrated;
  doc["mq7WarmupDone"] = p.readings.mq7WarmupDone;
  doc["mq7Level"] = p.readings.mq7Level;

  String body;
  serializeJson(doc, body);

  int code = http.POST(body);
  String resp = http.getString();
  http.end();

  if (code < 0) {
    Serial.printf("HTTP error: %d\n", code);
    return false;
  }

  Serial.printf("POST %d | %s\n", code, resp.c_str());
  return code >= 200 && code < 300;
}

} // namespace net
