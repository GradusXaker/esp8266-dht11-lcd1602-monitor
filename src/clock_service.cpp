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

  refreshWifiScan(true);
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
    refreshWifiScan();
    wifiConnected_ = false;
    timeValid_ = false;
    return;
  }

  const wl_status_t status = WiFi.status();
  wifiConnected_ = status == WL_CONNECTED;
  if (!wifiConnected_) {
    refreshWifiScan();
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

void ClockService::setTelemetry(const DeviceTelemetry& telemetry) {
  telemetry_ = telemetry;
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

bool ClockService::isBacklightEnabled() const {
  return config_.backlightEnabled;
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

String ClockService::getSensorLine() const {
  if (!telemetry_.sensorHasValidData) {
    return "Sensor offline";
  }

  String line = telemetry_.sensorOk ? "Sensor ok " : "Sensor stale ";
  line += String(telemetry_.sensorAgeMs / 1000);
  line += "s";
  return line;
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
  refreshWifiScan();
  String html;
  html.reserve(7000);
  html += "<!doctype html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>ESP8266 погодная станция</title>";
  html += "<style>";
  html += ":root{--bg:#eef4ff;--bg2:#dceaff;--ink:#10213a;--muted:#5f6f88;--card:#ffffff;--line:#d4deee;--accent:#0f7bff;--accent2:#19b8a6;--danger:#da516b;--shadow:0 18px 50px rgba(16,33,58,.10);}*{box-sizing:border-box;}";
  html += "body{margin:0;font-family:Verdana,sans-serif;color:var(--ink);background:radial-gradient(circle at top left,#ffffff 0, var(--bg) 42%, var(--bg2) 100%);}main{max-width:1080px;margin:0 auto;padding:28px 18px 56px;}";
  html += ".hero{position:relative;overflow:hidden;background:linear-gradient(135deg,#0f1f3b,#0d6efd 58%,#1db7a7);color:#fff;border-radius:24px;padding:28px;box-shadow:var(--shadow);} .hero:before{content:'';position:absolute;inset:auto -80px -80px auto;width:220px;height:220px;border-radius:50%;background:rgba(255,255,255,.10);}";
  html += ".hero h1{margin:0 0 10px;font-size:clamp(28px,4vw,42px);line-height:1.05;} .hero p{margin:0;max-width:720px;color:rgba(255,255,255,.86);} .badges{display:flex;flex-wrap:wrap;gap:10px;margin-top:18px;} .badge{padding:8px 12px;border:1px solid rgba(255,255,255,.18);border-radius:999px;background:rgba(255,255,255,.10);backdrop-filter:blur(8px);font-size:13px;}";
  html += ".grid{display:grid;grid-template-columns:repeat(12,1fr);gap:16px;margin-top:18px;} .card{grid-column:span 12;background:var(--card);border:1px solid var(--line);border-radius:20px;padding:18px;box-shadow:var(--shadow);} .card h2{margin:0 0 12px;font-size:18px;} .stat-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:12px;} .stat{padding:14px;border-radius:16px;background:linear-gradient(180deg,#f9fbff,#eef5ff);border:1px solid #dbe5f3;} .stat .label{display:block;font-size:12px;text-transform:uppercase;letter-spacing:.08em;color:var(--muted);} .stat .value{display:block;margin-top:8px;font-size:22px;font-weight:700;}";
  html += ".split{display:grid;grid-template-columns:1.2fr .8fr;gap:16px;} .display{display:grid;grid-template-columns:1fr 1fr;gap:12px;} .lcd{padding:18px;border-radius:18px;background:linear-gradient(180deg,#20310f,#111c08);color:#b8ff8f;font-family:monospace;border:1px solid #2d4218;box-shadow:inset 0 0 0 1px rgba(255,255,255,.04);} .lcd .line{font-size:18px;line-height:1.55;white-space:pre;}";
  html += "label{display:block;margin:12px 0 6px;font-size:13px;font-weight:700;color:#223554;} input{width:100%;padding:12px 13px;border-radius:12px;border:1px solid #cad7ea;background:#f8fbff;color:var(--ink);} .actions{display:flex;flex-wrap:wrap;gap:12px;margin-top:14px;} button{padding:12px 16px;border:0;border-radius:12px;background:linear-gradient(135deg,var(--accent),#2d8cff);color:#fff;font-weight:700;cursor:pointer;} .danger{background:linear-gradient(135deg,var(--danger),#bf3659);} .muted{color:var(--muted);} code{background:#eef3f8;padding:3px 7px;border-radius:7px;} .tiny{font-size:12px;} @media (max-width:860px){.split{grid-template-columns:1fr;} .display{grid-template-columns:1fr;} main{padding:18px 14px 40px;}}";
  html += "</style></head><body><main>";
  html += "<section class='hero'><h1>ESP8266 погодная станция</h1>";
  html += "<p>Живой статус устройства, настройка Wi-Fi, время по NTP, OTA и показания датчика в одной странице. Без bland-технической заглушки: сразу понятная и пригодная панель устройства.</p>";
  html += "<div class='badges'>";
  html += "<span class='badge'>Режим: <strong id='modeBadge'>";
  html += apMode_ ? "AP setup" : "Station";
  html += "</strong></span>";
  html += "<span class='badge'>IP: <strong id='ipBadge'>" + htmlEscape(getIpAddress()) + "</strong></span>";
  html += "<span class='badge'>OTA: <strong id='otaBadge'>" + String(otaEnabled_ ? "включено" : "ожидание") + "</strong></span>";
  html += "<span class='badge'>Подсветка: <strong id='blBadge'>" + String(config_.backlightEnabled ? "вкл" : "выкл") + "</strong></span>";
  html += "<span class='badge'>JSON: <code>/status</code></span></div></section>";
  html += "<div class='grid'>";
  html += "<section class='card'><h2>Живой статус</h2><div class='stat-grid'>";
  html += "<div class='stat'><span class='label'>Температура</span><span class='value' id='tempValue'>--.- C</span></div>";
  html += "<div class='stat'><span class='label'>Влажность</span><span class='value' id='humValue'>-- %</span></div>";
  html += "<div class='stat'><span class='label'>Wi-Fi</span><span class='value' id='wifiValue'>--</span></div>";
  html += "<div class='stat'><span class='label'>Время</span><span class='value' id='timeValue'>--:--:--</span></div>";
  html += "<div class='stat'><span class='label'>Свободная память</span><span class='value' id='heapValue'>--</span></div>";
  html += "<div class='stat'><span class='label'>Uptime</span><span class='value' id='uptimeValue'>--</span></div>";
  html += "<div class='stat'><span class='label'>Сенсор</span><span class='value' id='sensorInfoValue'>--</span></div>";
  html += "</div></section>";
  html += "<div class='split' style='grid-column:span 12'>";
  html += "<section class='card'><h2>LCD и устройство</h2><div class='display'>";
  html += "<div class='lcd'><div class='line' id='lcdLine1'>" + htmlEscape(getTimeLine()) + "</div><div class='line' id='lcdLine2'>" + htmlEscape(getStatusLine()) + "</div></div>";
  html += "<div class='muted'><p><strong>Hostname:</strong> <span id='hostnameValue'>" + htmlEscape(hostname_) + "</span></p>";
  html += "<p><strong>IP:</strong> <span id='ipValue'>" + htmlEscape(getIpAddress()) + "</span></p>";
  html += "<p><strong>RSSI:</strong> <span id='rssiValue'>--</span></p>";
  html += "<p><strong>LCD:</strong> <span id='lcdState'>--</span></p>";
  html += "<p><strong>Подсветка:</strong> <span id='backlightValue'>--</span></p>";
  html += "<p><strong>Датчик:</strong> <span id='sensorState'>--</span></p>";
  html += "<p><strong>Возраст данных:</strong> <span id='sensorAgeValue'>--</span></p>";
  html += "<p><strong>Chip ID:</strong> <span id='chipValue'>--</span></p></div></section>";
  html += "<section class='card'><h2>Настройка сети</h2>";
  html += "<p class='muted'>Сохраненные настройки имеют приоритет. Если конфигурации нет, устройство само поднимает <code>ESP8266-Setup</code>.</p>";
  html += "<form method='post' action='/save'>";
  html += "<label for='ssid'>Wi-Fi SSID</label><input id='ssid' name='ssid' value='" + htmlEscape(config_.ssid) + "'>";
  html += "<div class='muted tiny'>Найденные сети: ";
  html += wifiOptionsHtml_.length() > 0 ? wifiOptionsHtml_ : "сканирование...";
  html += "</div>";
  html += "<label for='password'>Пароль Wi-Fi</label><input id='password' name='password' type='password' value='" + htmlEscape(config_.password) + "'>";
  html += "<label for='utcHours'>Смещение UTC (часы)</label><input id='utcHours' name='utcHours' type='number' min='-12' max='14' value='";
  html += String(config_.utcOffsetSeconds / 3600);
  html += "'>";
  html += "<label for='backlightEnabled'>Подсветка LCD</label><input id='backlightEnabled' name='backlightEnabled' type='checkbox'";
  if (config_.backlightEnabled) {
    html += " checked";
  }
  html += ">";
  html += "<div class='actions'><button type='submit'>Сохранить и перезапустить</button></div></form>";
  html += "<form method='post' action='/reset'><div class='actions'><button class='danger' type='submit'>Очистить конфиг и перезапустить</button></div></form>";
  html += "<p class='muted tiny'>После сохранения устройство автоматически перезапустится и попытается подключиться к заданной сети.</p></section></div>";
  html += "</div>";
  html += "<script>const fmtMs=(ms)=>{const s=Math.floor(ms/1000);const h=Math.floor(s/3600);const m=Math.floor((s%3600)/60);const sec=s%60;return `${h}h ${m}m ${sec}s`;};";
  html += "const num=(v,d=0)=>typeof v==='number'&&!Number.isNaN(v)?v.toFixed(d):'--';";
  html += "async function refresh(){try{const r=await fetch('/status',{cache:'no-store'});const s=await r.json();";
  html += "document.getElementById('modeBadge').textContent=s.mode==='ap'?'AP setup':'Station';";
  html += "document.getElementById('ipBadge').textContent=s.ip||'0.0.0.0';";
  html += "document.getElementById('otaBadge').textContent=s.otaEnabled?'включено':'ожидание';";
  html += "document.getElementById('blBadge').textContent=s.backlightEnabled?'вкл':'выкл';";
  html += "document.getElementById('tempValue').textContent=s.sensorOk?`${num(s.temperatureC,1)} C`:'нет данных';";
  html += "document.getElementById('humValue').textContent=s.sensorOk?`${num(s.humidityPct,0)} %`:'нет данных';";
  html += "document.getElementById('wifiValue').textContent=s.wifiConnected?'подключен':(s.wifiConfigured?'подключение...':'не настроен');";
  html += "document.getElementById('timeValue').textContent=s.timeLine||'--:--:--';";
  html += "document.getElementById('heapValue').textContent=s.freeHeap?`${s.freeHeap} B`:'--';";
  html += "document.getElementById('uptimeValue').textContent=fmtMs(s.uptimeMs||0);";
  html += "document.getElementById('sensorInfoValue').textContent=s.sensorLine||'--';";
  html += "document.getElementById('lcdLine1').textContent=s.dateLine||s.timeLine||'--';";
  html += "document.getElementById('lcdLine2').textContent=s.networkLine||s.statusLine||'--';";
  html += "document.getElementById('hostnameValue').textContent=s.hostname||'--';";
  html += "document.getElementById('ipValue').textContent=s.ip||'0.0.0.0';";
  html += "document.getElementById('rssiValue').textContent=s.wifiConnected?`${s.rssi} dBm`:'--';";
  html += "document.getElementById('lcdState').textContent=s.lcdOk?`ok (0x${Number(s.lcdAddress).toString(16)})`:'ошибка';";
  html += "document.getElementById('backlightValue').textContent=s.backlightEnabled?'включена':'выключена';";
  html += "document.getElementById('sensorState').textContent=s.sensorHasValidData?(s.sensorOk?'ok':'устарели'):'ошибка';";
  html += "document.getElementById('sensorAgeValue').textContent=s.sensorHasValidData?fmtMs(s.sensorAgeMs||0):'--';";
  html += "document.getElementById('chipValue').textContent=s.chipId||'--';";
  html += "}catch(e){console.warn(e);}}refresh();setInterval(refresh,3000);</script>";
  html += "</main></body></html>";
  g_server.send(200, "text/html", html);
}

void ClockService::handleSave() {
  AppConfig next = config_;
  next.ssid = g_server.arg("ssid");
  next.password = g_server.arg("password");
  next.utcOffsetSeconds = g_server.arg("utcHours").toInt() * 3600L;
  next.backlightEnabled = g_server.hasArg("backlightEnabled");

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
  doc["backlightEnabled"] = config_.backlightEnabled;
  doc["apMode"] = apMode_;
  doc["lcdOk"] = telemetry_.lcdOk;
  doc["lcdAddress"] = telemetry_.lcdAddress;
  doc["sensorOk"] = telemetry_.sensorOk;
  doc["sensorHasValidData"] = telemetry_.sensorHasValidData;
  doc["temperatureC"] = telemetry_.temperatureC;
  doc["humidityPct"] = telemetry_.humidityPct;
  doc["sensorAgeMs"] = telemetry_.sensorAgeMs;
  doc["sensorLine"] = getSensorLine();
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
  doc["uptimeMs"] = telemetry_.uptimeMs;
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

void ClockService::refreshWifiScan(bool force) {
  const unsigned long now = millis();
  if (!force && (now - lastWifiScanMs_) < config::kWifiScanRefreshMs) {
    return;
  }
  lastWifiScanMs_ = now;

  const int networks = WiFi.scanNetworks();
  if (networks <= 0) {
    wifiOptionsHtml_ = "сети не найдены";
    return;
  }

  String options;
  options.reserve(networks * 24);
  const int count = networks > 8 ? 8 : networks;
  for (int i = 0; i < count; ++i) {
    if (i > 0) {
      options += ", ";
    }
    options += htmlEscape(WiFi.SSID(i));
  }
  wifiOptionsHtml_ = options;
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
