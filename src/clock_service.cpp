#include "clock_service.h"

#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>

#include "config.h"

namespace {
WiFiUDP g_ntpUdp;
NTPClient g_timeClient(g_ntpUdp);
ESP8266WebServer g_server(80);
}

void ClockService::begin() {
  configStore_.begin();
  config_.ssid = config::kWifiSsid;
  config_.password = config::kWifiPassword;
  config_.utcOffsetSeconds = config::kUtcOffsetSeconds;
  configStore_.load(config_);

  const uint32_t chipId = ESP.getChipId();
  char hostname[32];
  snprintf(hostname, sizeof(hostname), "%s-%06lx", config::kOtaHostnamePrefix,
           static_cast<unsigned long>(chipId));
  hostname_ = hostname;

  setupWebServer();
  if (config_.isWifiConfigured()) {
    startStation();
  } else {
    startAccessPoint();
  }
}

void ClockService::update() {
  if (!started_) {
    return;
  }

  g_server.handleClient();

  if (restartScheduled_ && millis() >= restartAtMs_) {
    ESP.restart();
  }

  if (otaEnabled_) {
    ArduinoOTA.handle();
  }

  if (apMode_) {
    wifiConnected_ = false;
    timeValid_ = false;
    return;
  }

  const wl_status_t status = WiFi.status();
  wifiConnected_ = status == WL_CONNECTED;
  if (!wifiConnected_) {
    timeValid_ = false;
    otaEnabled_ = false;
    if (millis() - lastReconnectAttemptMs_ >= config::kWifiReconnectIntervalMs) {
      lastReconnectAttemptMs_ = millis();
      Serial.println("[WIFI] Reconnecting...");
      WiFi.disconnect();
      WiFi.begin(config_.ssid.c_str(), config_.password.c_str());
    }
    return;
  }

  if (!otaEnabled_) {
    setupOta();
  }

  g_timeClient.setPoolServerName(config::kNtpServer);
  g_timeClient.setTimeOffset(config_.utcOffsetSeconds);
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
  return config_.isWifiConfigured();
}

bool ClockService::isWifiConnected() const {
  return wifiConnected_;
}

bool ClockService::hasValidTime() const {
  return timeValid_;
}

bool ClockService::isApMode() const {
  return apMode_;
}

bool ClockService::isOtaEnabled() const {
  return otaEnabled_;
}

bool ClockService::shouldShowInfoScreen() const {
  return apMode_ || isConfigured();
}

uint8_t ClockService::getScreenCount() const {
  if (apMode_) {
    return 2;
  }
  if (isConfigured()) {
    return 3;
  }
  return 1;
}

String ClockService::getTimeLine() const {
  if (apMode_) {
    return "Setup AP mode";
  }
  if (!timeValid_) {
    return "Time not ready";
  }
  return g_timeClient.getFormattedTime();
}

String ClockService::getDateLine() const {
  if (apMode_) {
    return "Open 192.168.4.1";
  }
  if (!timeValid_) {
    return getStatusLine();
  }

  const time_t epoch = static_cast<time_t>(g_timeClient.getEpochTime());
  const struct tm* tmInfo = gmtime(&epoch);
  if (!tmInfo) {
    return "Date not ready";
  }

  char buffer[17];
  const unsigned int day = static_cast<unsigned int>(tmInfo->tm_mday);
  const unsigned int month = static_cast<unsigned int>(tmInfo->tm_mon + 1);
  const unsigned int year = static_cast<unsigned int>(tmInfo->tm_year + 1900);
  snprintf(buffer, sizeof(buffer), "%02u.%02u.%04u", day, month, year);
  return String(buffer);
}

String ClockService::getStatusLine() const {
  if (apMode_) {
    return "192.168.4.1";
  }
  if (!isConfigured()) {
    return "WiFi disabled";
  }
  if (!wifiConnected_) {
    return "WiFi connect...";
  }
  if (!timeValid_) {
    return "NTP sync...";
  }
  return otaEnabled_ ? "Clock+OTA ready" : "Clock via NTP";
}

String ClockService::getIpAddress() const {
  if (apMode_) {
    return WiFi.softAPIP().toString();
  }
  if (wifiConnected_) {
    return WiFi.localIP().toString();
  }
  return "0.0.0.0";
}

String ClockService::getNetworkLine() const {
  if (apMode_) {
    return config::kSetupApSsid;
  }
  if (!wifiConnected_) {
    return "IP 0.0.0.0";
  }

  String line = "IP ";
  line += WiFi.localIP().toString();
  return line;
}

String ClockService::getHostname() const {
  return hostname_;
}

long ClockService::getUtcOffsetSeconds() const {
  return config_.utcOffsetSeconds;
}

int ClockService::getRssi() const {
  if (!wifiConnected_) {
    return 0;
  }
  return WiFi.RSSI();
}

void ClockService::startAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(config::kSetupApSsid);
  started_ = true;
  apMode_ = true;
  otaEnabled_ = false;
  Serial.printf("[WIFI] AP mode: %s at %s\n", config::kSetupApSsid,
                WiFi.softAPIP().toString().c_str());
}

void ClockService::startStation() {
  WiFi.mode(WIFI_STA);
  WiFi.hostname(hostname_);
  WiFi.begin(config_.ssid.c_str(), config_.password.c_str());
  started_ = true;
  apMode_ = false;
  otaEnabled_ = false;
  lastReconnectAttemptMs_ = millis();
  Serial.printf("[WIFI] Connecting to %s\n", config_.ssid.c_str());
}

void ClockService::setupWebServer() {
  if (webStarted_) {
    return;
  }

  g_server.on("/", HTTP_GET, [this]() { handleRoot(); });
  g_server.on("/save", HTTP_POST, [this]() { handleSave(); });
  g_server.on("/status", HTTP_GET, [this]() { handleStatus(); });
  g_server.on("/reset", HTTP_POST, [this]() { handleReset(); });
  g_server.onNotFound([this]() { handleStatus(); });
  g_server.begin();
  webStarted_ = true;
  Serial.println("[WEB] Server started on port 80");
}

void ClockService::setupOta() {
  ArduinoOTA.setHostname(hostname_.c_str());
  ArduinoOTA.onStart([]() { Serial.println("[OTA] Start"); });
  ArduinoOTA.onEnd([]() { Serial.println("[OTA] Done"); });
  ArduinoOTA.onError([](ota_error_t error) { Serial.printf("[OTA] Error: %u\n", error); });
  ArduinoOTA.begin();
  otaEnabled_ = true;
  Serial.printf("[OTA] Enabled as %s\n", hostname_.c_str());
}

void ClockService::handleRoot() {
  String html;
  html.reserve(2200);
  html += "<!doctype html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>ESP8266 Setup</title>";
  html += "<style>body{font-family:Verdana,sans-serif;max-width:760px;margin:24px auto;padding:0 16px;background:#f4f7fb;color:#142033;}";
  html += "h1{margin-bottom:8px;}section,form{background:#fff;border:1px solid #d8e2ef;border-radius:12px;padding:16px;margin:16px 0;}";
  html += "label{display:block;font-weight:700;margin:12px 0 6px;}input{width:100%;padding:10px;border:1px solid #c6d3e3;border-radius:8px;}button{margin-top:14px;padding:10px 14px;border:0;border-radius:8px;background:#1b6ef3;color:#fff;font-weight:700;}";
  html += ".muted{color:#5e7188;font-size:14px;} .danger{background:#d9485f;} code{background:#eef3f8;padding:2px 6px;border-radius:6px;}</style></head><body>";
  html += "<h1>ESP8266 weather setup</h1>";
  html += "<p class='muted'>Use this page to configure Wi-Fi, UTC offset, OTA, and time sync.</p>";
  html += "<section><strong>Mode:</strong> ";
  html += apMode_ ? "AP setup" : "Station";
  html += "<br><strong>IP:</strong> ";
  html += htmlEscape(getIpAddress());
  html += "<br><strong>Hostname:</strong> ";
  html += htmlEscape(hostname_);
  html += "<br><strong>Status:</strong> ";
  html += htmlEscape(getStatusLine());
  html += "<br><strong>JSON:</strong> <code>/status</code></section>";
  html += "<form method='post' action='/save'>";
  html += "<label for='ssid'>Wi-Fi SSID</label><input id='ssid' name='ssid' value='" + htmlEscape(config_.ssid) + "'>";
  html += "<label for='password'>Wi-Fi password</label><input id='password' name='password' type='password' value='" + htmlEscape(config_.password) + "'>";
  html += "<label for='utcHours'>UTC offset hours</label><input id='utcHours' name='utcHours' type='number' min='-12' max='14' value='";
  html += String(config_.utcOffsetSeconds / 3600);
  html += "'>";
  html += "<button type='submit'>Save and restart</button></form>";
  html += "<form method='post' action='/reset'><button class='danger' type='submit'>Clear config and restart</button></form>";
  html += "</body></html>";
  g_server.send(200, "text/html", html);
}

void ClockService::handleSave() {
  AppConfig next = config_;
  next.ssid = g_server.arg("ssid");
  next.password = g_server.arg("password");
  next.utcOffsetSeconds = g_server.arg("utcHours").toInt() * 3600L;

  if (!configStore_.save(next)) {
    g_server.send(500, "text/plain", "Failed to save config");
    return;
  }

  config_ = next;
  g_server.send(200, "text/plain", "Saved. Device will restart in 1.5s.");
  scheduleRestart();
}

void ClockService::handleStatus() {
  JsonDocument doc;
  doc["mode"] = apMode_ ? "ap" : "sta";
  doc["wifiConfigured"] = isConfigured();
  doc["wifiConnected"] = wifiConnected_;
  doc["timeSynced"] = timeValid_;
  doc["otaEnabled"] = otaEnabled_;
  doc["apMode"] = apMode_;
  doc["ip"] = getIpAddress();
  doc["hostname"] = hostname_;
  doc["ssid"] = config_.ssid;
  doc["utcOffsetSeconds"] = config_.utcOffsetSeconds;
  doc["rssi"] = getRssi();
  doc["screenCount"] = getScreenCount();
  doc["timeLine"] = getTimeLine();
  doc["dateLine"] = getDateLine();
  doc["networkLine"] = getNetworkLine();
  doc["statusLine"] = getStatusLine();
  doc["chipId"] = ESP.getChipId();
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["uptimeMs"] = millis();
  String json;
  serializeJson(doc, json);
  g_server.send(200, "application/json", json);
}

void ClockService::handleReset() {
  configStore_.clear();
  g_server.send(200, "text/plain", "Config cleared. Device will restart in 1.5s.");
  scheduleRestart();
}

void ClockService::scheduleRestart() {
  restartScheduled_ = true;
  restartAtMs_ = millis() + config::kRestartDelayMs;
}

String ClockService::htmlEscape(const String& value) const {
  String result;
  result.reserve(value.length() + 8);
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value[i];
    if (c == '&') {
      result += "&amp;";
    } else if (c == '<') {
      result += "&lt;";
    } else if (c == '>') {
      result += "&gt;";
    } else if (c == '"') {
      result += "&quot;";
    } else {
      result += c;
    }
  }
  return result;
}
