#include "app/hub.h"

#include "config.h"
#include "domain/calendar.h"

#include <Arduino.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

Hub::Hub(Network &network, ClockSource &clock, WeatherGateway &weather, Presenter &presenter)
  : network_(network),
    clock_(clock),
    weather_(weather),
    presenter_(presenter),
    forecast_(),
    wifiOk_(false),
    timeOk_(false),
    night_(false),
    shownSecond_(-1),
    nextWeatherAt_(0),
    nextWifiTry_(0) {}

bool Hub::bringWifi() {
  if (!network_.configured()) {
    Serial.println("Wi-Fi vide.");
    Serial.println("Ouvre config.h, remplis WIFI_SSID et WIFI_PASSWORD, puis téléverse à nouveau.");
    presenter_.showStatus("Wi-Fi manquant");
    return false;
  }

  Serial.print("Connexion a ");
  Serial.println(WIFI_SSID);
  presenter_.showStatus("Connexion...");
  if (!network_.connect()) {
    Serial.println("Wi-Fi refuse. Verifie le nom et le mot de passe dans config.h.");
    presenter_.showStatus("Wi-Fi echec");
    return false;
  }

  char address[20];
  network_.ip(address, sizeof(address));
  Serial.print("Wi-Fi ok : ");
  Serial.println(address);
  return true;
}

bool Hub::refreshWeather() {
  Forecast incoming;
  if (!weather_.fetch(CITY_LAT, CITY_LON, incoming)) return false;
  forecast_.temperatureC = incoming.temperatureC;
  forecast_.weatherCode = incoming.weatherCode;
  forecast_.weatherReady = true;
  if (incoming.sunKnown) {
    forecast_.sunriseMin = incoming.sunriseMin;
    forecast_.sunsetMin = incoming.sunsetMin;
    forecast_.sunKnown = true;
  } else if (!forecast_.sunKnown) {
    Serial.println("Soleil indisponible.");
  }
  return true;
}

void Hub::showNow() {
  struct tm now;
  if (!clock_.now(now, 10)) return;

  Frame frame;
  frame.city = CITY_NAME[0] ? CITY_NAME : "Paris";
  domain::formatClock(now, frame.clock, sizeof(frame.clock));
  domain::formatDate(now, frame.date, sizeof(frame.date));
  frame.temperatureC = (int)lroundf(forecast_.temperatureC);
  frame.weatherCode = forecast_.weatherCode;
  frame.weatherReady = forecast_.weatherReady;
  frame.sunKnown = forecast_.sunKnown;
  frame.night = domain::isNight(now, forecast_.sunKnown, forecast_.sunriseMin, forecast_.sunsetMin);
  frame.themeChanged = frame.night != night_;
  night_ = frame.night;
  presenter_.show(frame);
}

void Hub::begin() {
  presenter_.begin();
  Serial.print("Position : ");
  Serial.println(CITY_NAME[0] ? CITY_NAME : "Paris");

  wifiOk_ = bringWifi();
  if (!wifiOk_) {
    nextWifiTry_ = millis() + 30000UL;
    return;
  }

  timeOk_ = clock_.sync();
  if (!timeOk_) {
    Serial.println("Heure internet indisponible.");
    presenter_.showStatus("Heure indisponible");
  }

  if (refreshWeather()) nextWeatherAt_ = millis() + 600000UL;
  else nextWeatherAt_ = millis() + 60000UL;

  struct tm now;
  if (timeOk_ && clock_.now(now, 50)) {
    shownSecond_ = now.tm_sec;
    showNow();
  }
}

void Hub::tick() {
  unsigned long nowMs = millis();

  if (!wifiOk_) {
    if (!network_.configured()) {
      delay(1000);
      return;
    }
    if (nowMs >= nextWifiTry_) {
      nextWifiTry_ = nowMs + 30000UL;
      wifiOk_ = bringWifi();
      if (wifiOk_) timeOk_ = clock_.sync();
    }
    return;
  }

  if (!network_.connected()) {
    wifiOk_ = false;
    Serial.println("Wi-Fi coupe.");
    return;
  }

  if (!timeOk_) {
    timeOk_ = clock_.sync();
    if (!timeOk_) presenter_.showStatus("Heure indisponible");
  }

  if (timeOk_ && nowMs >= nextWeatherAt_) {
    if (refreshWeather()) nextWeatherAt_ = nowMs + 600000UL;
    else nextWeatherAt_ = nowMs + 60000UL;
  }

  struct tm now;
  if (!timeOk_ || !clock_.now(now, 10)) return;
  if (now.tm_sec == shownSecond_) return;
  shownSecond_ = now.tm_sec;
  showNow();
}
