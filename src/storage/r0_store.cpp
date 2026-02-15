#include "storage/r0_store.h"

#if defined(ESP32)
  #include <Preferences.h>
  static Preferences prefs;
#endif

void R0Store::begin() {
#if defined(ESP32)
  prefs.begin("envmon", false);
#endif
  inited_ = true;
}

float R0Store::load(float fallback) const {
#if defined(ESP32)
  if (!inited_) return fallback;
  return prefs.getFloat("mq7_r0", fallback);
#else
  return fallback;
#endif
}

void R0Store::save(float r0) {
#if defined(ESP32)
  if (!inited_) return;
  prefs.putFloat("mq7_r0", r0);
#else
  (void)r0;
#endif
}

bool R0Store::hasValue() const {
#if defined(ESP32)
  if (!inited_) return false;
  return prefs.isKey("mq7_r0");
#else
  return false;
#endif
}

void R0Store::clear() {
#if defined(ESP32)
  if (!inited_) return;
  prefs.remove("mq7_r0");
#endif
}
