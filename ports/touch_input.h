#pragma once

// Un releve du doigt. x et y sont des pixels de l'ecran, 320 x 240.
struct TouchSample {
  bool down;
  int x;
  int y;
};

// Entree du programme. L'ecran tactile en est une version, la console n'en a pas.
class TouchInput {
 public:
  virtual ~TouchInput() {}
  virtual void begin() = 0;
  virtual TouchSample sample() = 0;
};
