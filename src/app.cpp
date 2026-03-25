#include "app.h"

#include <Arduino.h>

#include "config.h"

void AppController::begin() {
  state_.bootAtMs = millis();
  state_.lcdOk = display_.begin();
  state_.lcdAddress = display_.getAddress();

  sensor_.begin();
  printStartupInfo();

  if (state_.lcdOk) {
    display_.showBootScreen();
  }
}

void AppController::loop() {
  updateSensor();
  updateDisplay();
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
    display_.showBootScreen();
    return;
  }
  state_.showBootScreen = false;

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
  Serial.println("[APP] Future-ready structure: Wi-Fi/NTP/OTA can be added later");
}
