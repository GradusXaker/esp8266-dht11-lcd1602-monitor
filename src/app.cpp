#include "app.h"

#include <Arduino.h>

#include "config.h"

void AppController::begin() {
  state_.bootAtMs = millis();
  state_.lcdOk = display_.begin();
  state_.lcdAddress = display_.getAddress();

  clock_.begin();
  updateClock();
  sensor_.begin();
  printStartupInfo();

  if (state_.lcdOk) {
    display_.showBootScreen(state_.lcdAddress);
  }
}

void AppController::loop() {
  updateClock();
  updateSensor();
  updateDisplay();
}

void AppController::updateClock() {
  clock_.update();
  state_.wifiConfigured = clock_.isConfigured();
  state_.wifiConnected = clock_.isWifiConnected();
  state_.timeSynced = clock_.hasValidTime();
  state_.apMode = clock_.isApMode();
  state_.otaEnabled = clock_.isOtaEnabled();
}

void AppController::updateSensor() {
  const unsigned long now = millis();
  if (now - state_.lastSensorReadMs < config::kSensorReadIntervalMs) {
    return;
  }

  state_.lastSensorReadMs = now;
  const SensorReading reading = sensor_.read();
  if (!reading.ok) {
    state_.sensorOk = false;
    return;
  }

  state_.sensorOk = true;
  state_.temperatureC = reading.temperatureC;
  state_.humidityPct = reading.humidityPct;
}

void AppController::updateDisplay() {
  if (!state_.lcdOk) {
    return;
  }

  const unsigned long now = millis();
  if (now - state_.lastDisplayRefreshMs < config::kDisplayRefreshIntervalMs) {
    return;
  }
  state_.lastDisplayRefreshMs = now;

  if (state_.showBootScreen && now - state_.bootAtMs < config::kBootScreenMs) {
    display_.showBootScreen(state_.lcdAddress);
    return;
  }
  state_.showBootScreen = false;

  const uint8_t screenCount = clock_.getScreenCount();
  if (screenCount > 1 && now - state_.lastScreenSwitchMs >= config::kScreenSwitchIntervalMs) {
    state_.lastScreenSwitchMs = now;
    state_.currentScreenIndex = (state_.currentScreenIndex + 1) % screenCount;
  }

  if (state_.currentScreenIndex == 1) {
    display_.showClockData(clock_.getTimeLine(), clock_.getStatusLine());
    return;
  }

  if (state_.currentScreenIndex == 2) {
    display_.showStatusData(clock_.getDateLine(), clock_.getNetworkLine());
    return;
  }

  if (!state_.sensorOk) {
    display_.showSensorError();
    return;
  }

  display_.showSensorData(state_.temperatureC, state_.humidityPct);
}

void AppController::printStartupInfo() const {
  Serial.println();
  Serial.println("[APP] ESP8266 weather monitor starting");
  Serial.printf("[APP] LCD ready: %s\n", state_.lcdOk ? "yes" : "no");
  if (state_.lcdOk) {
    Serial.printf("[APP] LCD address: 0x%02X\n", state_.lcdAddress);
  }
  Serial.printf("[APP] Sensor pin: D5 / GPIO %u\n", config::kDhtPin);
  Serial.printf("[APP] Wi-Fi configured: %s\n", state_.wifiConfigured ? "yes" : "no");
  Serial.printf("[APP] AP mode: %s\n", state_.apMode ? "yes" : "no");
  Serial.printf("[APP] Hostname: %s\n", clock_.getHostname().c_str());
  if (state_.wifiConfigured) {
    Serial.printf("[APP] NTP server: %s\n", config::kNtpServer);
  }
  Serial.println("[APP] OTA and web setup are available when network starts");
}
