#pragma once
#include <Arduino.h>

class R0Store {
public:
  void begin();
  float load(float fallback) const;
  void save(float r0);
  bool hasValue() const;
  void clear();

private:
  mutable bool inited_ = false;
};
