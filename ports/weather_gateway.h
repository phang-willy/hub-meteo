#pragma once

struct Forecast {
  float temperatureC = 0;
  int weatherCode = -1;
  bool weatherReady = false;
  bool sunKnown = false;
  int sunriseMin = 0;
  int sunsetMin = 0;
};

class WeatherGateway {
 public:
  virtual ~WeatherGateway() {}
  virtual bool fetch(float latitude, float longitude, Forecast &out) = 0;
};
