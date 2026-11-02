#pragma once

#include "ports/network.h"

class WifiLink : public Network {
 public:
  bool configured() const override;
  bool connect() override;
  bool connected() const override;
  void ip(char *out, size_t size) const override;
};
