#include "display/oled_display.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

static constexpr int SCREEN_W = 128;
static constexpr int SCREEN_H = 64;

static Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1); // -1 = no reset pin

void OledDisplay::begin() {
  Wire.begin(21,22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // 0x3C tipico
    // se non parte, prova 0x3D (vedi sotto)
    while (true) delay(1000);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("EnvMonitor");
  display.println("Booting...");
  display.display();
}

void OledDisplay::update(float tC, float rh, float coPpm, float ratio, bool mqOk, bool calibrated, bool warmupDone, uint8_t mq7Level) {
  const uint32_t now = millis();
  if (now < nextDraw_) return;
  nextDraw_ = now + 500; // refresh 2Hz

  display.clearDisplay();
  display.setCursor(0, 0);

  display.printf("T: %.1f C\n", tC);
  display.printf("RH: %.0f %%\n", rh);

  if (!warmupDone) {
    display.println("CO: warmup...");
    display.printf("ratio: %.2f\n", ratio);
  } else if (!calibrated) {
    display.println("CO: calib needed");
    display.printf("ratio: %.2f\n", ratio);
  } else if (mqOk) {
    display.printf("CO: %.0f ppm\n", coPpm);
    display.printf("ratio: %.2f\n", ratio);
  } else {
    display.println("CO: n/d");
    display.printf("ratio: %.2f\n", ratio);
  }

  const char* level = "UNK";
  if (mq7Level == 1) level = "OK";
  else if (mq7Level == 2) level = "WARN";
  else if (mq7Level == 3) level = "DANGER";
  display.printf("State: %s\n", level);

  display.display();
}
