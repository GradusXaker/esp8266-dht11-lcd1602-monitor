#pragma once

#include <Arduino.h>

struct AppState {
  bool lcdOk = false;
  bool sensorOk = false;
  bool showBootScreen = true;
  uint8_t lcdAddress = 0;
  float temperatureC = NAN;
  float humidityPct = NAN;
  unsigned long bootAtMs = 0;
  unsigned long lastSensorReadMs = 0;
  unsigned long lastDisplayRefreshMs = 0;
};
