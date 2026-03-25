#pragma once

#include <Arduino.h>

#include "config.h"

struct AppConfig {
  String ssid;
  String password;
  long utcOffsetSeconds = config::kUtcOffsetSeconds;
  bool backlightEnabled = true;

  bool isWifiConfigured() const {
    return ssid.length() > 0;
  }
};
