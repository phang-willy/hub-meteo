#include "adapters/ntp_clock.h"

#include "config.h"

#include <Arduino.h>

bool NtpClock::sync() {
  configTzTime(TIMEZONE, "pool.ntp.org", "time.google.com");
  struct tm now;
  for (int i = 0; i < 40; i++) {
    if (getLocalTime(&now, 250)) return true;
  }
  return false;
}

bool NtpClock::now(struct tm &out, unsigned long waitMs) {
  return getLocalTime(&out, waitMs);
}
