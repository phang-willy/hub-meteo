#pragma once

#include "domain/frame.h"

// Sortie du programme. La console et l'ecran en sont deux versions.
class Presenter {
 public:
  virtual ~Presenter() {}
  virtual void begin() = 0;
  virtual void showStatus(const char *line) = 0;
  virtual void show(const Frame &frame) = 0;
};
