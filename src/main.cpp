#include <Arduino.h>

#include "app.h"

namespace {
AppController app;
}

void setup() {
  Serial.begin(115200);
  delay(200);
  app.begin();
}

void loop() {
  app.loop();
}
