#include "drivers/heater_controller.h"

void HeaterController::begin(int pinHeater) {
  pin_ = pinHeater;
  pinMode(pin_, OUTPUT);

#if defined(ESP32)
  ledcSetup(pwmChannel_, pwmFreq_, pwmResBits_);
  ledcAttachPin(pin_, pwmChannel_);
#endif
  off();
}

void HeaterController::setHigh() {
#if defined(ESP32)
  ledcWrite(pwmChannel_, 255);  // ~100%
#else
  digitalWrite(pin_, HIGH);
#endif
}

void HeaterController::setLowEquivalent1V4() {
  // 1.4V/5V ≈ 0.28 duty
  const int duty = (int)(255.0f * (1.4f / 5.0f)); // ~71
#if defined(ESP32)
  ledcWrite(pwmChannel_, duty);
#else
  // su Arduino classico: PWM su pin ~ (non tutti)
  analogWrite(pin_, duty);
#endif
}

void HeaterController::off() {
#if defined(ESP32)
  ledcWrite(pwmChannel_, 0);
#else
  digitalWrite(pin_, LOW);
#endif
}