#pragma once

#include "app_state.h"
#include "display.h"
#include "sensor.h"

class AppController {
 public:
  void begin();
  void loop();

 private:
  void updateSensor();
  void updateDisplay();
  void printStartupInfo() const;

  AppState state_;
  DisplayManager display_;
  SensorManager sensor_;
};
