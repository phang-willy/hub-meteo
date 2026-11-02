#pragma once

namespace domain {

enum Icon {
  ICON_NONE = 0,
  ICON_SUN,
  ICON_CLOUD,
  ICON_RAIN,
  ICON_SNOW,
  ICON_STORM,
  ICON_FOG
};

inline const char *weatherLabel(int code) {
  if (code == 0) return "Ciel clair";
  if (code == 1) return "Peu nuageux";
  if (code == 2) return "Nuageux";
  if (code == 3) return "Couvert";
  if (code == 45 || code == 48) return "Brouillard";
  if (code >= 51 && code <= 57) return "Pluie Fine";
  if (code >= 61 && code <= 67) return "Pluie";
  if (code >= 71 && code <= 77) return "Neige";
  if (code == 80 || code == 81 || code == 82) return "Averses";
  if (code == 85 || code == 86) return "Neige";
  if (code >= 95) return "Orage";
  return "Meteo";
}

inline Icon weatherIcon(int code) {
  if (code == 0 || code == 1) return ICON_SUN;
  if (code == 2 || code == 3) return ICON_CLOUD;
  if (code == 45 || code == 48) return ICON_FOG;
  if (code >= 51 && code <= 67) return ICON_RAIN;
  if (code >= 71 && code <= 77) return ICON_SNOW;
  if (code == 80 || code == 81 || code == 82) return ICON_RAIN;
  if (code == 85 || code == 86) return ICON_SNOW;
  if (code >= 95) return ICON_STORM;
  return ICON_CLOUD;
}

}
