#include "config_store.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

#include "config.h"

bool ConfigStore::begin() {
  if (LittleFS.begin()) {
    return true;
  }

  Serial.println("[CFG] LittleFS mount failed");
  return false;
}

bool ConfigStore::load(AppConfig& config) {
  if (!LittleFS.exists(config::kConfigPath)) {
    Serial.println("[CFG] No saved config");
    return false;
  }

  File file = LittleFS.open(config::kConfigPath, "r");
  if (!file) {
    Serial.println("[CFG] Failed to open config for read");
    return false;
  }

  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error) {
    Serial.printf("[CFG] Parse error: %s\n", error.c_str());
    return false;
  }

  config.ssid = doc["ssid"] | "";
  config.password = doc["password"] | "";
  config.utcOffsetSeconds = doc["utcOffsetSeconds"] | config::kUtcOffsetSeconds;
  Serial.printf("[CFG] Loaded config, SSID set: %s\n", config.isWifiConfigured() ? "yes" : "no");
  return true;
}

bool ConfigStore::save(const AppConfig& config) {
  File file = LittleFS.open(config::kConfigPath, "w");
  if (!file) {
    Serial.println("[CFG] Failed to open config for write");
    return false;
  }

  JsonDocument doc;
  doc["ssid"] = config.ssid;
  doc["password"] = config.password;
  doc["utcOffsetSeconds"] = config.utcOffsetSeconds;

  if (serializeJson(doc, file) == 0) {
    file.close();
    Serial.println("[CFG] Failed to write config");
    return false;
  }

  file.close();
  Serial.println("[CFG] Config saved");
  return true;
}

bool ConfigStore::clear() {
  if (!LittleFS.exists(config::kConfigPath)) {
    return true;
  }
  const bool removed = LittleFS.remove(config::kConfigPath);
  Serial.printf("[CFG] Clear config: %s\n", removed ? "ok" : "failed");
  return removed;
}
