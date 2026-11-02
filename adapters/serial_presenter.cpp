#include "adapters/serial_presenter.h"

#include "domain/weather.h"

#include <Arduino.h>

void SerialPresenter::begin() {
  themeAnnounced_ = false;
}

void SerialPresenter::showStatus(const char *line) {
  (void)line;
}

void SerialPresenter::show(const Frame &frame) {
  if (frame.weatherReady) {
    Serial.print(frame.temperatureC);
    Serial.print("\u00B0C - ");
    Serial.println(domain::weatherLabel(frame.weatherCode));
  } else {
    Serial.println("Meteo...");
  }
  Serial.println(frame.city);
  Serial.println(frame.clock);
  Serial.println(frame.date);
  Serial.println();

  if (frame.sunKnown && (frame.themeChanged || !themeAnnounced_)) {
    themeAnnounced_ = true;
    Serial.println(frame.night ? "Theme : nuit" : "Theme : jour");
  }
}
