#include "adapters/backlight.h"

#if APP_MODE == MODE_SCREEN

#include <Arduino.h>
#include <Preferences.h>

namespace {

const int DAY_PERCENT = 75;
const int NIGHT_PERCENT = 25;
const int MIN_PERCENT = 5;
const int STEP_PERCENT = 5;

}

int Backlight::clamp(int percent) const {
  if (percent < MIN_PERCENT) return MIN_PERCENT;
  if (percent > 100) return 100;
  return percent;
}

void Backlight::apply(int percent) {
  applied_ = clamp(percent);
  ledcWrite(TFT_LED, (uint32_t)applied_ * 256 / 100);
}

void Backlight::begin() {
  pinMode(TFT_LED, OUTPUT);
  if (!ledcAttach(TFT_LED, 5000, 8)) Serial.println("Luminosite : broche indisponible");

  Preferences prefs;
  prefs.begin("hub", true);
  manual_ = clamp(prefs.getInt("level", 80));
  automatic_ = prefs.getBool("auto", true);
  prefs.end();

  apply(automatic_ ? DAY_PERCENT : manual_);
  Serial.println(automatic_ ? "Luminosite : auto" : "Luminosite : manuelle");
}

void Backlight::setManual(int percent) {
  automatic_ = false;
  manual_ = clamp(percent);
  apply(manual_);
}

void Backlight::nudge(int delta) {
  int next = (clamp(applied_ + delta) / STEP_PERCENT) * STEP_PERCENT;
  setManual(next);
}

void Backlight::setAutomatic(bool on) {
  automatic_ = on;
  if (on) apply(night_ ? NIGHT_PERCENT : DAY_PERCENT);
  else apply(manual_);
}

void Backlight::followNight(bool night) {
  night_ = night;
  if (!automatic_) return;
  apply(night ? NIGHT_PERCENT : DAY_PERCENT);
}

void Backlight::save() {
  Preferences prefs;
  if (!prefs.begin("hub", false)) return;
  prefs.putInt("level", manual_);
  prefs.putBool("auto", automatic_);
  prefs.end();
}

#endif
