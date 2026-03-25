#pragma once

#include <Arduino.h>

class DisplayManager {
 public:
  bool begin();
  bool isReady() const;
  uint8_t getAddress() const;
  void showLines(const String& line1, const String& line2);
  void showBootScreen();
  void showSensorData(float temperatureC, float humidityPct);
  void showSensorError();

 private:
  bool i2cDevicePresent(uint8_t address);
  uint8_t detectAddress();
  String fitLine(const String& line) const;

  uint8_t address_ = 0;
  String lastLine1_;
  String lastLine2_;
};
