#include "source/sd_logger.h"

#include "config.h"

#include <SPI.h>
#include <SD.h>
#include <math.h>

namespace {

void printCsvFloat(File& f, float v, uint8_t decimals) {
  if (!isnan(v)) {
    f.print(v, decimals);
  }
}

} // namespace

bool SdLogger::begin() {
  SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);

  ready_ = SD.begin(PIN_SD_CS);
  if (!ready_) {
    Serial.println("SD init failed");
    return false;
  }

  if (!ensureFileHasHeader_()) {
    Serial.println("SD header write failed");
    ready_ = false;
    return false;
  }

  nextWriteAtMs_ = millis();
  return true;
}

void SdLogger::update(uint32_t nowMs, const AppReadings& readings, uint32_t unixTs) {
  if (!ready_) return;
  if (nowMs < nextWriteAtMs_) return;

  nextWriteAtMs_ = nowMs + SD_PERIOD_MS;
  (void)appendNow(readings, unixTs);
}

bool SdLogger::appendNow(const AppReadings& readings, uint32_t unixTs) {
  if (!ready_) return false;

  File f = SD.open(SD_FILE_PATH, FILE_APPEND);
  if (!f) return false;

  f.print(unixTs);
  f.print(',');
  f.print(millis());
  f.print(',');
  printCsvFloat(f, readings.tC, 2);
  f.print(',');
  printCsvFloat(f, readings.rh, 2);
  f.print(',');
  f.print(readings.dhtOk ? 1 : 0);
  f.print(',');
  f.print(readings.mq7Raw);
  f.print(',');
  printCsvFloat(f, readings.mq7Ratio, 4);
  f.print(',');
  printCsvFloat(f, readings.mq7Ppm, 1);
  f.print(',');
  printCsvFloat(f, readings.mq7R0, 1);
  f.print(',');
  f.print(readings.mq7Ok ? 1 : 0);
  f.print(',');
  f.print(readings.mq7Calibrated ? 1 : 0);
  f.print(',');
  f.print(readings.mq7WarmupDone ? 1 : 0);
  f.print(',');
  f.println(readings.mq7Level);

  f.close();
  return true;
}

bool SdLogger::ensureFileHasHeader_() {
  if (SD.exists(SD_FILE_PATH)) return true;

  File f = SD.open(SD_FILE_PATH, FILE_WRITE);
  if (!f) return false;

  f.println("ts,millis,tC,rh,dhtOk,mq7Raw,mq7Ratio,mq7Ppm,mq7R0,mq7Ok,mq7Calibrated,mq7WarmupDone,mq7Level");
  f.close();
  return true;
}
