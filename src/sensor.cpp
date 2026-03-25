#include "sensor.h"

#include <Arduino.h>
#include <DHT.h>

#include "config.h"

namespace {
DHT g_dht(config::kDhtPin, config::kDhtType);
}

void SensorManager::begin() {
  g_dht.begin();
  Serial.printf("[DHT] Initialized on GPIO %u\n", config::kDhtPin);
}

SensorReading SensorManager::read() {
  SensorReading reading;

  const float humidity = g_dht.readHumidity();
  const float temperature = g_dht.readTemperature();
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("[DHT] Read failed");
    return reading;
  }

  reading.ok = true;
  reading.temperatureC = temperature;
  reading.humidityPct = humidity;

  Serial.printf("[DHT] Temp: %.1f C | Hum: %.0f %%\n", temperature, humidity);
  return reading;
}
