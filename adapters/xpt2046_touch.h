#pragma once

#include "config.h"

#if APP_MODE == MODE_SCREEN

#include "ports/touch_input.h"

// XPT2046 du module 3.2 pouces. Son bus est separe de celui de l'ecran.
class Xpt2046Touch : public TouchInput {
 public:
  void begin() override;
  TouchSample sample() override;

 private:
  bool readRaw(int *rawX, int *rawY);

  bool ready_ = false;
  bool stable_ = false;
  bool candidate_ = false;
  int same_ = 0;
  int pointX_ = 0;
  int pointY_ = 0;
  unsigned long lastPoll_ = 0;
  TouchSample last_{false, 0, 0};
};

#endif
