#pragma once

#include <time.h>

class ClockSource {
 public:
  virtual ~ClockSource() {}
  virtual bool sync() = 0;
  virtual bool now(struct tm &out, unsigned long waitMs) = 0;
};
