#pragma once

#include <Arduino.h>

class DisplayManager {
 public:
  bool begin();
  bool isReady() const;
  uint8_t getAddress() const;
  void showLines(const String& line1, const String& line2);
  void showBootScreen(uint8_t lcdAddress);
  void showSensorData(float temperatureC, float humidityPct);
  void showSensorError();
  void showClockData(const String& timeLine, const String& statusLine);
  void showStatusData(const String& line1, const String& line2);

 private:
  bool i2cDevicePresent(uint8_t address);
  uint8_t detectAddress();
  String fitLine(const String& line) const;
  void writeLine(uint8_t row, const String& line);

  uint8_t address_ = 0;
  String lastLine1_;
  String lastLine2_;
};
