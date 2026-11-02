#pragma once

#include "ports/clock_source.h"

class NtpClock : public ClockSource {
 public:
  bool sync() override;
  bool now(struct tm &out, unsigned long waitMs) override;
};
