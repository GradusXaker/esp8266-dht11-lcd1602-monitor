#pragma once

#include <Arduino.h>

class ClockService {
 public:
  void begin();
  void update();
  bool isConfigured() const;
  bool isWifiConnected() const;
  bool hasValidTime() const;
  String getTimeLine() const;
  String getStatusLine() const;

 private:
  bool started_ = false;
  bool wifiConnected_ = false;
  bool timeValid_ = false;
  unsigned long lastReconnectAttemptMs_ = 0;
};
