#include "adapters/xpt2046_touch.h"

#if APP_MODE == MODE_SCREEN

#include <Arduino.h>
#include <SPI.h>

namespace {

const int Z_MIN = 400;
const unsigned long POLL_MS = 12;
const int STEADY_READS = 2;

SPIClass &bus() {
  static SPIClass spi(HSPI);
  return spi;
}

void chipSelect(bool active) {
  digitalWrite(TOUCH_CS, active ? LOW : HIGH);
}

void powerDown() {
  SPIClass &spi = bus();
  spi.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
  chipSelect(true);
  spi.transfer(0xD0);
  spi.transfer16(0);
  chipSelect(false);
  spi.endTransaction();
}

int mapRange(int raw, int minRaw, int maxRaw, int size, bool invert) {
  if (raw < minRaw) raw = minRaw;
  if (raw > maxRaw) raw = maxRaw;
  int span = maxRaw - minRaw;
  if (span <= 0 || size <= 1) return 0;
  int value = (int)((long)(raw - minRaw) * (size - 1) / span);
  if (invert) value = (size - 1) - value;
  if (value < 0) return 0;
  if (value >= size) return size - 1;
  return value;
}

}

void Xpt2046Touch::begin() {
  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);
  pinMode(TOUCH_IRQ, INPUT_PULLUP);
  ready_ = bus().begin(TOUCH_CLK, TOUCH_DO, TOUCH_DIN, TOUCH_CS);
  digitalWrite(TOUCH_CS, HIGH);
  if (!ready_) {
    Serial.println("Tactile : bus indisponible");
    return;
  }
  powerDown();
  Serial.println("Tactile pret");
}

bool Xpt2046Touch::readRaw(int *rawX, int *rawY) {
  if (digitalRead(TOUCH_IRQ) != LOW) return false;

  SPIClass &spi = bus();
  spi.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
  chipSelect(true);
  spi.transfer(0xB1);
  int16_t z1 = spi.transfer16(0xC1) >> 3;
  int z = z1 + 4095;
  int16_t z2 = spi.transfer16(0x91) >> 3;
  z -= z2;

  int data0 = 0;
  int data1 = 0;
  int data2 = 0;
  int data3 = 0;
  if (z >= Z_MIN) {
    spi.transfer16(0x91);
    data0 = spi.transfer16(0xD1) >> 3;
    data1 = spi.transfer16(0x91) >> 3;
    data2 = spi.transfer16(0xD1) >> 3;
    data3 = spi.transfer16(0x91) >> 3;
  }
  spi.transfer16(0xD0);
  spi.transfer16(0);
  chipSelect(false);
  spi.endTransaction();
  if (z < Z_MIN) return false;

  *rawX = (data0 + data2) / 2;
  *rawY = (data1 + data3) / 2;
  return true;
}

TouchSample Xpt2046Touch::sample() {
  if (!ready_) return TouchSample{false, 0, 0};

  unsigned long now = millis();
  if (now - lastPoll_ < POLL_MS) return last_;
  lastPoll_ = now;

  int rawX = 0;
  int rawY = 0;
  bool raw = readRaw(&rawX, &rawY);
  if (raw == candidate_) {
    if (same_ < STEADY_READS) same_++;
  } else {
    candidate_ = raw;
    same_ = 1;
  }
  if (same_ >= STEADY_READS) stable_ = candidate_;
  if (raw) {
    pointX_ = mapRange(rawX, TOUCH_X_MIN, TOUCH_X_MAX, 320, TOUCH_INVERT_X != 0);
    pointY_ = mapRange(rawY, TOUCH_Y_MIN, TOUCH_Y_MAX, 240, TOUCH_INVERT_Y != 0);
  }

  last_.down = stable_;
  last_.x = pointX_;
  last_.y = pointY_;
  return last_;
}

#endif
