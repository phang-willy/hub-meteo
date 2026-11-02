#pragma once

#include "ports/presenter.h"

// Envoie le meme cadre a la console et, en mode ecran, a l'afficheur.
class CompositePresenter : public Presenter {
 public:
  CompositePresenter() : count_(0) {}

  void add(Presenter *presenter) {
    if (presenter && count_ < 2) items_[count_++] = presenter;
  }

  void begin() override {
    for (int i = 0; i < count_; i++) items_[i]->begin();
  }

  void showStatus(const char *line) override {
    for (int i = 0; i < count_; i++) items_[i]->showStatus(line);
  }

  void show(const Frame &frame) override {
    for (int i = 0; i < count_; i++) items_[i]->show(frame);
  }

 private:
  Presenter *items_[2];
  int count_;
};
