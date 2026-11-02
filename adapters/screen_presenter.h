#pragma once

#include "config.h"

#if APP_MODE == MODE_SCREEN

#include "ports/presenter.h"

class ScreenPresenter : public Presenter {
 public:
  ScreenPresenter();
  ~ScreenPresenter();
  void begin() override;
  void showStatus(const char *line) override;
  void show(const Frame &frame) override;

 private:
  struct Impl;
  Impl *impl_;
};

#endif
