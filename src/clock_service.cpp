#include "clock_service.h"

#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "config.h"

namespace {
WiFiUDP g_ntpUdp;
NTPClient g_timeClient(g_ntpUdp, config::kNtpServer, config::kUtcOffsetSeconds, 60000);
}

void ClockService::begin() {
  if (!isConfigured()) {
    Serial.println("[WIFI] Disabled, SSID is empty");
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(config::kWifiSsid, config::kWifiPassword);
  started_ = true;
  lastReconnectAttemptMs_ = millis();
  Serial.printf("[WIFI] Connecting to %s\n", config::kWifiSsid);
}

void ClockService::update() {
  if (!started_) {
    return;
  }

  const wl_status_t status = WiFi.status();
  wifiConnected_ = status == WL_CONNECTED;
  if (!wifiConnected_) {
    timeValid_ = false;
    if (millis() - lastReconnectAttemptMs_ >= config::kWifiReconnectIntervalMs) {
      lastReconnectAttemptMs_ = millis();
      Serial.println("[WIFI] Reconnecting...");
      WiFi.disconnect();
      WiFi.begin(config::kWifiSsid, config::kWifiPassword);
    }
    return;
  }

  if (!g_timeClient.isTimeSet()) {
    g_timeClient.begin();
  }

  if (g_timeClient.update() || g_timeClient.forceUpdate()) {
    if (!timeValid_) {
      Serial.printf("[NTP] Time synced: %s\n", g_timeClient.getFormattedTime().c_str());
    }
    timeValid_ = true;
  }
}

bool ClockService::isConfigured() const {
  return config::isWifiConfigured();
}

bool ClockService::isWifiConnected() const {
  return wifiConnected_;
}

bool ClockService::hasValidTime() const {
  return timeValid_;
}

String ClockService::getTimeLine() const {
  if (!timeValid_) {
    return "Time not ready";
  }

  return g_timeClient.getFormattedTime();
}

String ClockService::getStatusLine() const {
  if (!isConfigured()) {
    return "WiFi disabled";
  }
  if (!wifiConnected_) {
    return "WiFi connect...";
  }
  if (!timeValid_) {
    return "NTP sync...";
  }
  return "Clock via NTP";
}
