#pragma once

#include "ports/weather_gateway.h"

class OpenMeteo : public WeatherGateway {
 public:
  bool fetch(float latitude, float longitude, Forecast &out) override;
};
