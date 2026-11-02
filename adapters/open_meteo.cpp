#include "adapters/open_meteo.h"

#include "domain/weather.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {

bool jsonNumber(const char *json, const char *key, float *out) {
  const char *current = strstr(json, "\"current\":");
  if (!current) return false;
  const char *field = strstr(current, key);
  if (!field) return false;
  const char *colon = strchr(field, ':');
  if (!colon) return false;
  char *end = nullptr;
  float value = strtof(colon + 1, &end);
  if (end == colon + 1) return false;
  *out = value;
  return true;
}

bool parseHm(const char *text, int *minutes) {
  if (text[0] < '0' || text[0] > '9' || text[1] < '0' || text[1] > '9' || text[2] != ':') return false;
  if (text[3] < '0' || text[3] > '9' || text[4] < '0' || text[4] > '9') return false;
  int hour = (text[0] - '0') * 10 + (text[1] - '0');
  int minute = (text[3] - '0') * 10 + (text[4] - '0');
  if (hour > 23 || minute > 59) return false;
  *minutes = hour * 60 + minute;
  return true;
}

bool jsonSunTime(const char *json, const char *key, int *minutes) {
  const char *field = json;
  while ((field = strstr(field, key)) != nullptr) {
    const char *cursor = field + strlen(key);
    while (*cursor == ' ' || *cursor == '\t') cursor++;
    if (*cursor == '[') {
      const char *mark = strchr(cursor, 'T');
      if (mark && (mark - cursor) < 24 && parseHm(mark + 1, minutes)) return true;
    }
    field += strlen(key);
  }
  return false;
}

bool readBody(HTTPClient &http, char *body, int capacity) {
  WiFiClient *stream = http.getStreamPtr();
  int n = 0;
  unsigned long start = millis();
  while (millis() - start < 8000 && n < capacity - 1) {
    while (stream->available() && n < capacity - 1) body[n++] = (char)stream->read();
    if (!http.connected() && !stream->available()) break;
    delay(10);
  }
  body[n] = '\0';
  return n > 0;
}

}

bool OpenMeteo::fetch(float latitude, float longitude, Forecast &out) {
  char lat[16];
  char lon[16];
  char url[320];
  dtostrf(latitude, 0, 4, lat);
  dtostrf(longitude, 0, 4, lon);
  snprintf(
    url,
    sizeof(url),
    "https://api.open-meteo.com/v1/forecast?latitude=%s&longitude=%s&current=temperature_2m,weather_code&daily=sunrise,sunset&forecast_days=1&timezone=Europe%%2FParis",
    lat,
    lon
  );

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(8000);
  if (!http.begin(client, url)) {
    Serial.println("Meteo: connexion impossible.");
    return false;
  }

  int status = http.GET();
  if (status != 200) {
    Serial.print("Meteo HTTP ");
    Serial.println(status);
    http.end();
    return false;
  }

  static char body[2400];
  bool gotBody = readBody(http, body, (int)sizeof(body));
  http.end();
  if (!gotBody) return false;

  float temp = 0;
  float code = 0;
  if (!jsonNumber(body, "\"temperature_2m\"", &temp) || !jsonNumber(body, "\"weather_code\"", &code)) {
    Serial.println("Meteo: reponse illisible.");
    return false;
  }

  out.temperatureC = temp;
  out.weatherCode = (int)code;
  out.weatherReady = true;
  out.sunKnown = false;

  int rise = 0;
  int set = 0;
  if (jsonSunTime(body, "\"sunrise\":", &rise) && jsonSunTime(body, "\"sunset\":", &set) && rise < set) {
    out.sunriseMin = rise;
    out.sunsetMin = set;
    out.sunKnown = true;
    Serial.printf("Soleil : %02d:%02d - %02d:%02d\n", rise / 60, rise % 60, set / 60, set % 60);
  }

  Serial.print("Meteo ok : ");
  Serial.print((int)lroundf(out.temperatureC));
  Serial.print(" C, ");
  Serial.println(domain::weatherLabel(out.weatherCode));
  return true;
}
