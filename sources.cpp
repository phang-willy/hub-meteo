// Arduino ne compile que les .cpp a la racine du sketch.
// Les implementations restent dans leurs dossiers.

#include "app/hub.cpp"
#include "adapters/wifi_link.cpp"
#include "adapters/ntp_clock.cpp"
#include "adapters/open_meteo.cpp"
#include "adapters/serial_presenter.cpp"
#include "adapters/screen_presenter.cpp"
