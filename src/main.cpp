#include <Arduino.h>
#include <math.h>
#include "config.h"
#include "sensors/dht_sensor.h"
#include "sensors/mq7_sensor.h"
#include "app/app_state.h"
#include "net/wifi_manager.h"
#include "net/telemetry_client.h"
#include "time/time_sync.h"
#include "display/oled_display.h"
#include "source/sd_logger.h"

SdLogger sd;
OledDisplay oled;
DhtSensor dht;
Mq7Sensor mq7;
static uint32_t nextSendMs = 0;
static bool buzzerOn = false;
static uint32_t buzzerNextToggleMs = 0;
static constexpr int BUZZER_PWM_CHANNEL = 1;

enum class AlarmLevel : uint8_t { UNKNOWN = 0, OK = 1, WARN = 2, DANGER = 3 };
static AlarmLevel alarmLevel = AlarmLevel::UNKNOWN;

static void setBuzzer(bool on) {
  buzzerOn = on;

#if defined(ESP32)
  if (BUZZER_USE_TONE) {
    if (!on) {
      ledcWriteTone(BUZZER_PWM_CHANNEL, 0);
      return;
    }

    uint32_t freq = (alarmLevel == AlarmLevel::DANGER) ? BUZZER_DANGER_FREQ_HZ : BUZZER_WARN_FREQ_HZ;
    ledcWriteTone(BUZZER_PWM_CHANNEL, freq);
    return;
  }
#endif

  bool out = BUZZER_ACTIVE_HIGH ? on : !on;
  digitalWrite(PIN_BUZZER, out ? HIGH : LOW);
}

static AlarmLevel computeAlarmLevel(const Mq7Reading& mr) {
  if (!mr.warmupDone || !mr.calibrated || !isfinite(mr.ratio)) return AlarmLevel::UNKNOWN;
  if (mr.ratio < MQ7_RATIO_DANGER_LT) return AlarmLevel::DANGER;
  if (mr.ratio < MQ7_RATIO_WARN_LT) return AlarmLevel::WARN;
  return AlarmLevel::OK;
}

static void updateBuzzer(uint32_t nowMs, AlarmLevel level) {
  if (level != alarmLevel) {
    alarmLevel = level;
    setBuzzer(false);
    buzzerNextToggleMs = nowMs;
  }

  if (alarmLevel == AlarmLevel::UNKNOWN || alarmLevel == AlarmLevel::OK) {
    setBuzzer(false);
    return;
  }

  if (nowMs < buzzerNextToggleMs) return;

  if (alarmLevel == AlarmLevel::WARN) {
    if (!buzzerOn) {
      setBuzzer(true);
      buzzerNextToggleMs = nowMs + BUZZER_WARN_ON_MS;
    } else {
      setBuzzer(false);
      buzzerNextToggleMs = nowMs + (BUZZER_WARN_PERIOD_MS - BUZZER_WARN_ON_MS);
    }
    return;
  }

  // DANGER: beep continuo a impulsi rapidi.
  setBuzzer(!buzzerOn);
  buzzerNextToggleMs = nowMs + (buzzerOn ? BUZZER_DANGER_ON_MS : BUZZER_DANGER_OFF_MS);
}

void setup() {
  Serial.begin(115200);
  oled.begin();
  delay(300);
  sd.begin();

#if defined(ESP32)
  if (BUZZER_USE_TONE) {
    ledcSetup(BUZZER_PWM_CHANNEL, BUZZER_WARN_FREQ_HZ, 8);
    ledcAttachPin(PIN_BUZZER, BUZZER_PWM_CHANNEL);
  } else {
    pinMode(PIN_BUZZER, OUTPUT);
  }
#else
  pinMode(PIN_BUZZER, OUTPUT);
#endif
  setBuzzer(false);

  net::wifiBegin();
  timeutil::beginNtp();
  dht.begin();
  mq7.begin();

  Serial.println("EnvMonitor start");
  Serial.println("Type 'c' + Enter to calibrate MQ7 R0 (in clean air, during LOW phase).");
  Serial.println("Type 'r' + Enter to reset MQ7 R0 calibration.");
  Serial.println("Calibration uses 20 LOW samples and requires stable signal (<5% stddev).");
  Serial.println("MQ7 readings are ignored for the first 10 minutes (warm-up).");
}

void loop() {
  uint32_t now = millis();

  net::wifiEnsureConnected(now);
  dht.update(now);
  mq7.update(now);

  // Serial commands
  if (Serial.available()) {
    char ch = (char)Serial.read();
    if (ch == 'c') {
      bool ok = mq7.calibrateNow(MQ7_CALIB_SAMPLES);
      Serial.printf("MQ7 calibrate: %s\n", ok ? "OK" : "FAIL (need LOW phase + warm-up + stable clean air)");
    } else if (ch == 'r') {
      mq7.resetCalibration();
      Serial.println("MQ7 calibration reset (R0 fallback restored).");
    }
  }

  // Print (debug)
  auto dr = dht.get();
  if (dr.ok) {
    Serial.printf("DHT T=%.1fC RH=%.0f%% | ", dr.tC, dr.rh);
  } else {
    Serial.print("DHT fail | ");
  }

  auto mr = mq7.get();
  AlarmLevel level = computeAlarmLevel(mr);
  updateBuzzer(now, level);

  if (mr.ok) {
    const char* levelText = (level == AlarmLevel::DANGER) ? "DANGER" :
                            (level == AlarmLevel::WARN) ? "WARN" :
                            (level == AlarmLevel::OK) ? "OK" : "UNKNOWN";
    Serial.printf("MQ7 raw=%u vNode=%.3f Rs=%.0f R0=%.0f cal=%s warm=%s ratio=%.3f state=%s CO~%.0fppm\n",
                  mr.raw, mr.vNode, mr.rs, mr.r0, mr.calibrated ? "Y" : "N",
                  mr.warmupDone ? "Y" : "N", mr.ratio, levelText, mr.ppm);
  } else {
    Serial.printf("MQ7 raw=%u vNode=%.3f R0=%.0f cal=%s warm=%s (warming/low signal)\n",
                  mr.raw, mr.vNode, mr.r0, mr.calibrated ? "Y" : "N", mr.warmupDone ? "Y" : "N");
  }

  oled.update(dr.tC, dr.rh, mr.ppm, mr.ratio, mr.ok, mr.calibrated, mr.warmupDone, static_cast<uint8_t>(level));

  if (now >= nextSendMs) {
    nextSendMs = now + SEND_PERIOD_MS;

    AppReadings readings;
    readings.tC = dr.tC;
    readings.rh = dr.rh;
    readings.dhtOk = dr.ok;
    readings.mq7Raw = mr.raw;
    readings.mq7Ratio = mr.ratio;
    readings.mq7Ppm = mr.ppm;
    readings.mq7R0 = mr.r0;
    readings.mq7Ok = mr.ok;
    readings.mq7Calibrated = mr.calibrated;
    readings.mq7WarmupDone = mr.warmupDone;
    readings.mq7Level = static_cast<uint8_t>(level);

    net::TelemetryPayload payload{readings, timeutil::unixTime()};
    bool sent = net::postTelemetry(payload);
    if (!sent) {
      Serial.println("Telemetry send failed/skipped");
    }

    sd.update(now, readings, payload.ts);

  }

  delay(250);
}
