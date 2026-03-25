#pragma once

#include "app_state.h"
#include "clock_service.h"
#include "display.h"
#include "sensor.h"

class AppController {
 public:
  void begin();
  void loop();

 private:
  void updateClock();
  void updateSensor();
  void updateDisplay();
  void printStartupInfo() const;

  AppState state_;
  ClockService clock_;
  DisplayManager display_;
  SensorManager sensor_;
};
