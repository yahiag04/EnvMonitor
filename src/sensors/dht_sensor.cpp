#include "config.h"
#include "sensors/dht_sensor.h"
#include <DHT.h>

static DHT dht(PIN_DHT, DHT11); // cambia a DHT22 se serve

void DhtSensor::begin() {
  dht.begin();
  nextRead_ = 0;
}

void DhtSensor::update(uint32_t nowMs) {
  if (nowMs < nextRead_) return;
  nextRead_ = nowMs + DHT_PERIOD_MS;

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    last_ = {NAN, NAN, false};
  } else {
    last_ = {t, h, true};
  }
}