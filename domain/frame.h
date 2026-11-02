#pragma once

// Ce que l'ecran et la console affichent. Aucun des deux ne va chercher la meteo.
struct Frame {
  const char *city;
  char clock[16];
  char date[40];
  int temperatureC;
  int weatherCode;
  bool weatherReady;
  bool sunKnown;
  bool night;
  bool themeChanged;
};
