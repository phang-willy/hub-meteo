#pragma once

#include <stddef.h>
#include <stdio.h>
#include <time.h>

namespace domain {

inline void formatClock(const struct tm &now, char *out, size_t size) {
  snprintf(out, size, "%02d:%02d:%02d", now.tm_hour, now.tm_min, now.tm_sec);
}

inline void formatDate(const struct tm &now, char *out, size_t size) {
  const char *days[] = {"Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam"};
  const char *months[] = {"jan", "fev", "mar", "avr", "mai", "juin", "juil", "aout", "sept", "oct", "nov", "dec"};
  snprintf(out, size, "%s %d %s %d", days[now.tm_wday], now.tm_mday, months[now.tm_mon], now.tm_year + 1900);
}

// Nuit avant le lever et a partir du coucher. Sans horaires, on reste en jour.
inline bool isNight(const struct tm &now, bool sunKnown, int sunriseMin, int sunsetMin) {
  if (!sunKnown) return false;
  int mins = now.tm_hour * 60 + now.tm_min;
  return mins < sunriseMin || mins >= sunsetMin;
}

}
