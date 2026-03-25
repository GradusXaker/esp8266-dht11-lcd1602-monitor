#pragma once

#include <Arduino.h>

#include "app_config.h"
#include "config_store.h"

class ClockService {
 public:
  void begin();
  void update();
  bool isConfigured() const;
  bool isWifiConnected() const;
  bool hasValidTime() const;
  bool isApMode() const;
  bool isOtaEnabled() const;
  bool shouldShowInfoScreen() const;
  String getTimeLine() const;
  String getStatusLine() const;
  String getIpAddress() const;
  String getHostname() const;

 private:
  void startAccessPoint();
  void startStation();
  void setupWebServer();
  void setupOta();
  void handleRoot();
  void handleSave();
  void handleStatus();
  void handleReset();
  void scheduleRestart();
  String htmlEscape(const String& value) const;

  AppConfig config_;
  ConfigStore configStore_;
  bool started_ = false;
  bool apMode_ = false;
  bool wifiConnected_ = false;
  bool timeValid_ = false;
  bool otaEnabled_ = false;
  bool webStarted_ = false;
  bool restartScheduled_ = false;
  unsigned long lastReconnectAttemptMs_ = 0;
  unsigned long restartAtMs_ = 0;
  String hostname_;
};
