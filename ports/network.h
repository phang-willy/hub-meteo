#pragma once

#include <stddef.h>

class Network {
 public:
  virtual ~Network() {}
  virtual bool configured() const = 0;
  virtual bool connect() = 0;
  virtual bool connected() const = 0;
  virtual void ip(char *out, size_t size) const = 0;
};
