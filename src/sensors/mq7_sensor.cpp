#include "config.h"
#include "sensors/mq7_sensor.h"
#include <math.h>

static constexpr int AVG_N = 20;

void Mq7Sensor::begin() {
  store_.begin();
  calibrated_ = store_.hasValue();
  r0_ = store_.load(MQ7_R0_DEFAULT);
  warmupUntilMs_ = millis() + MQ7_WARMUP_MS;
  nextSampleAtMs_ = millis();

  // ADC: segnali bassi -> 0dB per sensibilità vicino allo zero
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_MQ7_ADC, ADC_0db);
}

uint16_t Mq7Sensor::readAvgRaw_() const {
  uint32_t sum = 0;
  for (int i = 0; i < AVG_N; i++) {
    sum += analogRead(PIN_MQ7_ADC);
    delay(3);
  }
  return (uint16_t)(sum / AVG_N);
}

float Mq7Sensor::rawToVnode_(uint16_t raw) const {
  // ADC_0db: ~0..1.1V nominal, ma Vref varia tra chip -> è una stima
  const float vref = 1.1f;
  return (raw * vref) / 4095.0f;
}

float Mq7Sensor::computeRs_(float vRl) const {
  // Datasheet: Rs/RL = (Vc - VRL) / VRL  => Rs = RL*(Vc - VRL)/VRL  [oai_citation:3‡SparkFun Electronics](https://www.sparkfun.com/datasheets/Sensors/Biometric/MQ-7.pdf?utm_source=chatgpt.com)
  if (vRl <= 0.0001f) return NAN;
  if (vRl >= VCC_SENSOR) vRl = VCC_SENSOR - 0.0001f;
  return RL_OHMS * (VCC_SENSOR - vRl) / vRl;
}

float Mq7Sensor::ratioToPpm_(float ratio) const {
  if (!(ratio > 0)) return NAN;
  // ppm = A * ratio^B (placeholder: fit da fare bene sulla tua curva)
  return CO_A * powf(ratio, CO_B);
}

void Mq7Sensor::update(uint32_t nowMs) {
  if (nowMs < nextSampleAtMs_) return;
  nextSampleAtMs_ = nowMs + MQ7_PERIOD_MS;

  uint16_t raw = readAvgRaw_();
  float vNode = rawToVnode_(raw);
  float vRl = vNode * DIVIDER_GAIN;     // ricostruisci VRL (prima del partitore)
  float rs = computeRs_(vRl);
  float ratio = (isnan(rs) || !(r0_ > 0)) ? NAN : (rs / r0_);
  float ppm = ratioToPpm_(ratio);
  bool warmupDone = isWarmupDone(nowMs);

  last_.raw = raw;
  last_.vNode = vNode;
  last_.vRl = vRl;
  last_.rs = rs;
  last_.r0 = r0_;
  last_.calibrated = calibrated_;
  last_.warmupDone = warmupDone;
  last_.ratio = ratio;
  last_.ppm = ppm;
  last_.ok = warmupDone && !isnan(ppm);
}

bool Mq7Sensor::calibrateNow(uint8_t samples) {
  // Calibra R0 usando Rs attuale (devi essere in aria pulita)
  if (!isWarmupDone(millis())) return false;
  if (samples == 0) return false;

  float rsValues[32];
  if (samples > 32) samples = 32;
  uint8_t rsCount = 0;
  for (uint8_t i = 0; i < samples; i++) {
    uint16_t raw = readAvgRaw_();
    float vNode = rawToVnode_(raw);
    float vRl = vNode * DIVIDER_GAIN;
    float rs = computeRs_(vRl);
    if (!isnan(rs) && rs > 0) {
      rsValues[rsCount++] = rs;
    }
    delay(10);
  }

  if (rsCount < 2) return false;

  float rsSum = 0.0f;
  for (uint8_t i = 0; i < rsCount; i++) rsSum += rsValues[i];
  float mean = rsSum / rsCount;
  if (!(mean > 0)) return false;

  float var = 0.0f;
  for (uint8_t i = 0; i < rsCount; i++) {
    float d = rsValues[i] - mean;
    var += d * d;
  }
  var /= rsCount;
  float stddev = sqrtf(var);
  float relStddev = stddev / mean;
  if (relStddev > MQ7_CALIB_MAX_REL_STDDEV) return false;

  r0_ = mean;
  calibrated_ = true;
  store_.save(r0_);
  return true;
}

void Mq7Sensor::resetCalibration() {
  store_.clear();
  calibrated_ = false;
  r0_ = MQ7_R0_DEFAULT;
}
