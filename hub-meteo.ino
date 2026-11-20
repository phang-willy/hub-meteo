// Branche les pieces. Le reste du programme ne sait pas si la sortie est l'ecran ou la console.
//
// domain/    heure, meteo, jour ou nuit
// ports/     ce dont le programme a besoin, sans dire comment
// adapters/  wifi, NTP, Open-Meteo, console, ecran, tactile
// app/       quand rafraichir, puis quoi montrer

#include "app/hub.h"
#include "adapters/ntp_clock.h"
#include "adapters/open_meteo.h"
#include "adapters/null_touch.h"
#include "adapters/screen_presenter.h"
#include "adapters/serial_presenter.h"
#include "adapters/wifi_link.h"
#include "adapters/xpt2046_touch.h"
#include "config.h"
#include "ports/composite_presenter.h"

#include <Arduino.h>

WifiLink wifiLink;
NtpClock ntpClock;
OpenMeteo openMeteo;
SerialPresenter serialPresenter;
#if APP_MODE == MODE_SCREEN
ScreenPresenter screenPresenter;
Xpt2046Touch touch;
#else
NullTouch touch;
#endif
CompositePresenter presenter;
Hub hub(wifiLink, ntpClock, openMeteo, presenter, touch);

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println();
  presenter.add(&serialPresenter);
#if APP_MODE == MODE_SCREEN
  presenter.add(&screenPresenter);
  Serial.println("Hub meteo - ecran");
#else
  Serial.println("Hub meteo - console");
  Serial.println("Ecran plus tard : dans config.h, mets APP_MODE a MODE_SCREEN.");
#endif
  hub.begin();
}

void loop() {
  hub.tick();
}
