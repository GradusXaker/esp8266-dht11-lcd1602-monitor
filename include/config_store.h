#pragma once

#include "app_config.h"

class ConfigStore {
 public:
  bool begin();
  bool load(AppConfig& config);
  bool save(const AppConfig& config);
  bool clear();
};
