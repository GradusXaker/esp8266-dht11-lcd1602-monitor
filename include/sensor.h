#pragma once

#include <Arduino.h>

struct SensorReading {
  bool ok = false;
  float temperatureC = NAN;
  float humidityPct = NAN;
};

class SensorManager {
 public:
  void begin();
  SensorReading read();
};
