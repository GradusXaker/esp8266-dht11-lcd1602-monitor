#pragma once

#include <Arduino.h>
#include <DHT.h>

namespace config {
constexpr uint8_t kLcdCols = 16;
constexpr uint8_t kLcdRows = 2;
constexpr uint8_t kLcdPrimaryAddress = 0x27;
constexpr uint8_t kLcdSecondaryAddress = 0x3F;

constexpr uint8_t kDhtPin = D5;
constexpr uint8_t kDhtType = DHT11;

constexpr unsigned long kSensorReadIntervalMs = 2500;
constexpr unsigned long kDisplayRefreshIntervalMs = 500;
constexpr unsigned long kBootScreenMs = 1500;
constexpr float kInvalidTemperature = -1000.0f;
constexpr float kInvalidHumidity = -1.0f;
}  // namespace config
