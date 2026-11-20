#pragma once

#include "domain/frame.h"
#include "ports/clock_source.h"
#include "ports/network.h"
#include "ports/presenter.h"
#include "ports/touch_input.h"
#include "ports/weather_gateway.h"

// Decide quand relire l'heure et la meteo, puis demande a la sortie de les montrer.
class Hub {
 public:
  Hub(Network &network, ClockSource &clock, WeatherGateway &weather, Presenter &presenter, TouchInput &touch);
  void begin();
  void tick();

 private:
  bool bringWifi();
  bool refreshWeather();
  void showNow();
  void pollTouch();

  Network &network_;
  ClockSource &clock_;
  WeatherGateway &weather_;
  Presenter &presenter_;
  TouchInput &touch_;
  Forecast forecast_;
  bool wifiOk_;
  bool timeOk_;
  bool night_;
  bool touchArmed_;
  int shownSecond_;
  unsigned long nextWeatherAt_;
  unsigned long nextWifiTry_;
  unsigned long touchReadyAt_;
};
