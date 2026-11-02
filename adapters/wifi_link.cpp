#include "adapters/wifi_link.h"

#include "config.h"

#include <Arduino.h>
#include <WiFi.h>
#include <stdio.h>
#include <string.h>

bool WifiLink::configured() const {
  return WIFI_SSID[0] != '\0';
}

bool WifiLink::connect() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();
  return WiFi.status() == WL_CONNECTED;
}

bool WifiLink::connected() const {
  return WiFi.status() == WL_CONNECTED;
}

void WifiLink::ip(char *out, size_t size) const {
  snprintf(out, size, "%s", WiFi.localIP().toString().c_str());
}
