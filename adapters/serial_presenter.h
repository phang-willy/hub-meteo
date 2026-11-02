#pragma once

#include "ports/presenter.h"

class SerialPresenter : public Presenter {
 public:
  void begin() override;
  void showStatus(const char *line) override;
  void show(const Frame &frame) override;

 private:
  bool themeAnnounced_ = false;
};
