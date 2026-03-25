#pragma once

#include <Arduino.h>

struct AppState {
  bool lcdOk = false;
  bool sensorOk = false;
  bool showBootScreen = true;
  bool showClockScreen = false;
  bool wifiConfigured = false;
  bool wifiConnected = false;
  bool timeSynced = false;
  bool apMode = false;
  bool otaEnabled = false;
  uint8_t lcdAddress = 0;
  float temperatureC = NAN;
  float humidityPct = NAN;
  unsigned long bootAtMs = 0;
  unsigned long lastScreenSwitchMs = 0;
  unsigned long lastSensorReadMs = 0;
  unsigned long lastDisplayRefreshMs = 0;
};
