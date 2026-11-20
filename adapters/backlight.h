#pragma once

#include "config.h"

#if APP_MODE == MODE_SCREEN

// Retroéclairage de l'ecran. 5 est le minimum, 100 est le plein feu.
// En automatique, le jour est a 75 et la nuit a 25.
class Backlight {
 public:
  void begin();
  void setManual(int percent);
  void nudge(int delta);
  void setAutomatic(bool on);
  void followNight(bool night);
  void save();
  int percent() const { return applied_; }
  bool automatic() const { return automatic_; }

 private:
  int clamp(int percent) const;
  void apply(int percent);

  int manual_ = 80;
  int applied_ = 75;
  bool automatic_ = true;
  bool night_ = false;
};

#endif
