#pragma once

#include <Arduino.h>

#include "app_config.h"
#include "config_store.h"

struct DeviceTelemetry {
  bool lcdOk = false;
  bool sensorOk = false;
  bool sensorHasValidData = false;
  uint8_t lcdAddress = 0;
  float temperatureC = NAN;
  float humidityPct = NAN;
  unsigned long uptimeMs = 0;
  unsigned long sensorAgeMs = 0;
};

class ClockService {
 public:
  void begin();
  void update();
  void setTelemetry(const DeviceTelemetry& telemetry);
  bool isConfigured() const;
  bool isWifiConnected() const;
  bool hasValidTime() const;
  bool isApMode() const;
  bool isOtaEnabled() const;
  bool isBacklightEnabled() const;
  bool shouldShowInfoScreen() const;
  uint8_t getScreenCount() const;
  String getTimeLine() const;
  String getDateLine() const;
  String getStatusLine() const;
  String getNetworkLine() const;
  String getIpAddress() const;
  String getHostname() const;
  long getUtcOffsetSeconds() const;
  int getRssi() const;
  String getSensorLine() const;

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
  void refreshWifiScan(bool force = false);
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
  unsigned long lastWifiScanMs_ = 0;
  unsigned long restartAtMs_ = 0;
  String hostname_;
  String wifiOptionsHtml_;
  DeviceTelemetry telemetry_;
};
