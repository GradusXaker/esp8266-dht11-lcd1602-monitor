#include "display.h"

#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#include "config.h"

namespace {
LiquidCrystal_I2C* g_lcd = nullptr;
}

bool DisplayManager::begin() {
  Wire.begin();
  address_ = detectAddress();
  if (address_ == 0) {
    Serial.println("[LCD] I2C display not found");
    return false;
  }

  g_lcd = new LiquidCrystal_I2C(address_, config::kLcdCols, config::kLcdRows);
  g_lcd->init();
  g_lcd->backlight();
  Serial.printf("[LCD] Ready at 0x%02X\n", address_);
  return true;
}

bool DisplayManager::isReady() const {
  return g_lcd != nullptr;
}

uint8_t DisplayManager::getAddress() const {
  return address_;
}

void DisplayManager::showLines(const String& line1, const String& line2) {
  if (!g_lcd) {
    return;
  }

  const String fittedLine1 = fitLine(line1);
  const String fittedLine2 = fitLine(line2);
  const bool line1Changed = fittedLine1 != lastLine1_;
  const bool line2Changed = fittedLine2 != lastLine2_;
  if (!line1Changed && !line2Changed) {
    return;
  }

  if (line1Changed) {
    writeLine(0, fittedLine1);
  }
  if (line2Changed) {
    writeLine(1, fittedLine2);
  }

  lastLine1_ = fittedLine1;
  lastLine2_ = fittedLine2;
}

void DisplayManager::showBootScreen(uint8_t lcdAddress) {
  char addressLine[17];
  snprintf(addressLine, sizeof(addressLine), "LCD 0x%02X ready", lcdAddress);
  showLines("ESP8266 Boot", String(addressLine));
}

void DisplayManager::showSensorData(float temperatureC, float humidityPct) {
  String line1 = "Temp: ";
  line1 += String(temperatureC, 1);
  line1 += " C";

  String line2 = "Hum:  ";
  line2 += String(humidityPct, 0);
  line2 += " %";

  showLines(line1, line2);
}

void DisplayManager::showSensorError() {
  showLines("DHT11 error", "Check wiring");
}

void DisplayManager::showClockData(const String& timeLine, const String& statusLine) {
  showLines(timeLine, statusLine);
}

bool DisplayManager::i2cDevicePresent(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

uint8_t DisplayManager::detectAddress() {
  if (i2cDevicePresent(config::kLcdPrimaryAddress)) {
    return config::kLcdPrimaryAddress;
  }
  if (i2cDevicePresent(config::kLcdSecondaryAddress)) {
    return config::kLcdSecondaryAddress;
  }

  for (uint8_t address = 8; address < 120; ++address) {
    if (i2cDevicePresent(address)) {
      return address;
    }
  }

  return 0;
}

String DisplayManager::fitLine(const String& line) const {
  String result = line;
  if (result.length() > config::kLcdCols) {
    result = result.substring(0, config::kLcdCols);
  }
  while (result.length() < config::kLcdCols) {
    result += ' ';
  }
  return result;
}

void DisplayManager::writeLine(uint8_t row, const String& line) {
  g_lcd->setCursor(0, row);
  g_lcd->print(line);
}
