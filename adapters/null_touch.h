#pragma once

#include "ports/touch_input.h"

// Mode console : aucun doigt a lire.
class NullTouch : public TouchInput {
 public:
  void begin() override {}
  TouchSample sample() override { return TouchSample{false, 0, 0}; }
};
