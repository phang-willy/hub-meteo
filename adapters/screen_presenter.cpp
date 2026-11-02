#include "adapters/screen_presenter.h"

#if APP_MODE == MODE_SCREEN

#include "domain/weather.h"

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include "fonts/GoogleSansBold52.h"
#include "fonts/GoogleSansRegular15.h"
#include "fonts/GoogleSansRegular13.h"
#include "fonts/GoogleSansRegular11.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

namespace {

const int BAND_WEATHER_Y = 4;
const int BAND_WEATHER_H = 30;
const int BAND_CITY_Y = 36;
const int BAND_CITY_H = 18;
const int BAND_CLOCK_Y = 58;
const int BAND_CLOCK_H = 144;
const int BAND_DATE_Y = 208;
const int BAND_DATE_H = 24;
const int ICON_H = 26;

}

struct ScreenPresenter::Impl {
  struct ClockLayout {
    bool ready;
    int textSize;
    int baseline;
    int blitY;
    int glyphH;
    int slotX[8];
    int slotW[8];
    GFXcanvas16 *digit;
    GFXcanvas16 *colon;
  };

  Adafruit_ILI9341 tft;
  bool night;
  ClockLayout clockLayout;
  bool painted;
  bool weatherPainted;
  bool weatherReadyPainted;
  int tempPainted;
  int codePainted;
  char datePainted[40];
  char clockPainted[16];

  Impl()
    : tft(TFT_CS, TFT_DC, TFT_RST),
      night(false),
      clockLayout{false, 1, 0, 0, 0, {0}, {0}, nullptr, nullptr},
      painted(false),
      weatherPainted(false),
      weatherReadyPainted(false),
      tempPainted(0),
      codePainted(-1) {
    datePainted[0] = '\0';
    clockPainted[0] = '\0';
  }

  uint16_t bg() const { return night ? ILI9341_BLACK : ILI9341_WHITE; }
  uint16_t ink() const { return night ? ILI9341_WHITE : ILI9341_BLACK; }

  void drawCentered(const char *text, int bandY, int bandH) {
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    int top = bandY + (bandH - (int)h) / 2;
    tft.setCursor((320 - (int)w) / 2 - x1, top - y1);
    tft.print(text);
  }

  void drawCloud(int x, int y) {
    tft.drawCircle(x + 8, y + 11, 5, ink());
    tft.drawCircle(x + 15, y + 8, 6, ink());
    tft.drawCircle(x + 21, y + 11, 5, ink());
    tft.fillRect(x + 5, y + 12, 20, 8, bg());
    tft.drawLine(x + 4, y + 16, x + 25, y + 16, ink());
  }

  void drawIcon(int x, int y, int icon) {
    if (icon == domain::ICON_SUN) {
      tft.drawCircle(x + 13, y + 12, 5, ink());
      tft.drawLine(x + 13, y + 1, x + 13, y + 4, ink());
      tft.drawLine(x + 13, y + 20, x + 13, y + 23, ink());
      tft.drawLine(x + 2, y + 12, x + 5, y + 12, ink());
      tft.drawLine(x + 21, y + 12, x + 24, y + 12, ink());
      tft.drawLine(x + 5, y + 4, x + 7, y + 6, ink());
      tft.drawLine(x + 19, y + 6, x + 21, y + 4, ink());
      tft.drawLine(x + 5, y + 20, x + 7, y + 18, ink());
      tft.drawLine(x + 19, y + 18, x + 21, y + 20, ink());
      return;
    }
    if (icon == domain::ICON_FOG) {
      tft.drawLine(x + 4, y + 6, x + 22, y + 6, ink());
      tft.drawLine(x + 2, y + 11, x + 24, y + 11, ink());
      tft.drawLine(x + 6, y + 16, x + 20, y + 16, ink());
      tft.drawLine(x + 4, y + 21, x + 22, y + 21, ink());
      return;
    }
    if (icon == domain::ICON_NONE) return;

    drawCloud(x, y);
    if (icon == domain::ICON_RAIN) {
      tft.drawLine(x + 8, y + 18, x + 6, y + 24, ink());
      tft.drawLine(x + 14, y + 18, x + 12, y + 24, ink());
      tft.drawLine(x + 20, y + 18, x + 18, y + 24, ink());
    } else if (icon == domain::ICON_SNOW) {
      tft.drawPixel(x + 8, y + 20, ink());
      tft.drawPixel(x + 14, y + 22, ink());
      tft.drawPixel(x + 20, y + 20, ink());
      tft.drawPixel(x + 11, y + 19, ink());
      tft.drawPixel(x + 17, y + 23, ink());
    } else if (icon == domain::ICON_STORM) {
      tft.drawLine(x + 14, y + 17, x + 10, y + 22, ink());
      tft.drawLine(x + 10, y + 22, x + 15, y + 22, ink());
      tft.drawLine(x + 15, y + 22, x + 11, y + 26, ink());
    }
  }

  void drawStatus(const char *line) {
    tft.fillScreen(bg());
    tft.setFont(&FreeSansBold12pt7b);
    tft.setTextSize(1);
    tft.setTextColor(ink());
    drawCentered(line, 90, 60);
    painted = false;
  }

  void begin() {
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin(20000000);
    tft.setRotation(TFT_ROTATION);
    tft.invertDisplay(TFT_INVERT);
    tft.fillScreen(bg());
    tft.setTextWrap(false);
  }

  void drawWeather(const Frame &frame) {
    tft.fillRect(0, BAND_WEATHER_Y, 320, BAND_WEATHER_H, bg());
    tft.setFont(&GoogleSansRegular15);
    tft.setTextSize(1);
    tft.setTextColor(ink());
    if (!frame.weatherReady) {
      drawCentered("Meteo...", BAND_WEATHER_Y, BAND_WEATHER_H);
      return;
    }

    char temp[8];
    char rest[40];
    snprintf(temp, sizeof(temp), "%d", frame.temperatureC);
    snprintf(rest, sizeof(rest), "C - %s", domain::weatherLabel(frame.weatherCode));

    int16_t tx, ty, rx, ry;
    uint16_t tw, th, rw, rh;
    (void)ry;
    (void)rh;
    tft.getTextBounds(temp, 0, 0, &tx, &ty, &tw, &th);
    tft.getTextBounds(rest, 0, 0, &rx, &ry, &rw, &rh);

    const int degree = 4;
    const int iconW = 26;
    int total = (int)tw + 2 + degree + 3 + (int)rw + 8 + iconW;
    int x = (320 - total) / 2;
    int top = BAND_WEATHER_Y + (BAND_WEATHER_H - (int)th) / 2;
    int baseline = top - ty;

    tft.setCursor(x - tx, baseline);
    tft.print(temp);
    x += (int)tw + 2;
    tft.drawCircle(x + degree / 2, top + 1, degree / 2, ink());
    x += degree + 3;
    tft.setCursor(x - rx, baseline);
    tft.print(rest);
    x += (int)rw + 8;
    drawIcon(x, BAND_WEATHER_Y + (BAND_WEATHER_H - ICON_H) / 2, domain::weatherIcon(frame.weatherCode));
  }

  void drawCity(const Frame &frame) {
    tft.fillRect(0, BAND_CITY_Y, 320, BAND_CITY_H, bg());
    tft.setFont(&GoogleSansRegular11);
    tft.setTextSize(1);
    tft.setTextColor(ink());
    drawCentered(frame.city, BAND_CITY_Y, BAND_CITY_H);
  }

  void drawDate(const char *date) {
    tft.fillRect(0, BAND_DATE_Y, 320, BAND_DATE_H, bg());
    tft.setFont(&GoogleSansRegular13);
    tft.setTextSize(1);
    tft.setTextColor(ink());
    drawCentered(date, BAND_DATE_Y, BAND_DATE_H);
  }

  int inkWidth(char c) {
    char text[2] = {c, '\0'};
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    return (int)w > 0 ? (int)w : 1;
  }

  bool prepareClock() {
    if (clockLayout.ready) {
      return clockLayout.digit && clockLayout.digit->getBuffer() && clockLayout.colon && clockLayout.colon->getBuffer();
    }

    tft.setFont(&GoogleSansBold52);
    tft.setTextWrap(false);
    tft.setTextSize(1);

    int digitW = 1;
    for (char c = '0'; c <= '9'; c++) {
      int width = inkWidth(c);
      if (width > digitW) digitW = width;
    }
    int colonW = inkWidth(':');
    int gap = 0;
    int total = 6 * digitW + 2 * colonW;
    for (int tryGap = 2; tryGap >= 0; tryGap--) {
      int packed = 6 * digitW + 2 * colonW + 7 * tryGap;
      if (packed <= 316) {
        gap = tryGap;
        total = packed;
        break;
      }
    }

    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds("8", 0, 0, &x1, &y1, &w, &h);
    clockLayout.textSize = 1;
    clockLayout.glyphH = (int)h;
    clockLayout.blitY = BAND_CLOCK_Y + (BAND_CLOCK_H - clockLayout.glyphH) / 2;
    clockLayout.baseline = -y1;

    int cursor = (320 - total) / 2;
    for (int i = 0; i < 8; i++) {
      bool colon = (i == 2 || i == 5);
      clockLayout.slotW[i] = colon ? colonW : digitW;
      clockLayout.slotX[i] = cursor;
      cursor += clockLayout.slotW[i];
      if (i < 7) cursor += gap;
    }

    clockLayout.digit = new GFXcanvas16((uint16_t)digitW, (uint16_t)clockLayout.glyphH);
    clockLayout.colon = new GFXcanvas16((uint16_t)colonW, (uint16_t)clockLayout.glyphH);
    clockLayout.ready = true;
    return clockLayout.digit && clockLayout.digit->getBuffer() && clockLayout.colon && clockLayout.colon->getBuffer();
  }

  void paintSlot(int index, char c) {
    bool colon = (index == 2 || index == 5);
    GFXcanvas16 *canvas = colon ? clockLayout.colon : clockLayout.digit;
    int slotW = clockLayout.slotW[index];
    char text[2] = {c, '\0'};
    int16_t x1, y1;
    uint16_t w, h;
    canvas->fillScreen(bg());
    canvas->setFont(&GoogleSansBold52);
    canvas->setTextWrap(false);
    canvas->setTextSize(clockLayout.textSize);
    canvas->setTextColor(ink());
    canvas->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    canvas->setCursor((slotW - (int)w) / 2 - x1, clockLayout.baseline);
    canvas->print(c);
    tft.drawRGBBitmap(clockLayout.slotX[index], clockLayout.blitY, canvas->getBuffer(), slotW, clockLayout.glyphH);
  }

  void drawClock(const char *clock) {
    if (!prepareClock()) {
      tft.fillRect(0, BAND_CLOCK_Y, 320, BAND_CLOCK_H, bg());
      tft.setFont(&GoogleSansBold52);
      tft.setTextColor(ink());
      tft.setTextSize(clockLayout.textSize);
      drawCentered(clock, BAND_CLOCK_Y, BAND_CLOCK_H);
      return;
    }

    bool full = clockPainted[0] == '\0';
    for (int i = 0; i < 8 && clock[i] != '\0'; i++) {
      if (!full && clockPainted[i] == clock[i]) continue;
      paintSlot(i, clock[i]);
    }
  }

  void show(const Frame &frame) {
    night = frame.night;
    if (frame.themeChanged) {
      painted = false;
      weatherPainted = false;
      datePainted[0] = '\0';
      clockPainted[0] = '\0';
    }

    if (!painted) {
      tft.fillScreen(bg());
      drawCity(frame);
      painted = true;
    }

    bool weatherChanged = !weatherPainted || weatherReadyPainted != frame.weatherReady;
    if (frame.weatherReady && (tempPainted != frame.temperatureC || codePainted != frame.weatherCode)) weatherChanged = true;
    if (weatherChanged) {
      drawWeather(frame);
      weatherPainted = true;
      weatherReadyPainted = frame.weatherReady;
      tempPainted = frame.temperatureC;
      codePainted = frame.weatherCode;
    }

    if (strcmp(datePainted, frame.date) != 0) {
      drawDate(frame.date);
      strncpy(datePainted, frame.date, sizeof(datePainted) - 1);
      datePainted[sizeof(datePainted) - 1] = '\0';
    }

    if (strcmp(clockPainted, frame.clock) != 0) {
      drawClock(frame.clock);
      strncpy(clockPainted, frame.clock, sizeof(clockPainted) - 1);
      clockPainted[sizeof(clockPainted) - 1] = '\0';
    }
  }
};

ScreenPresenter::ScreenPresenter() : impl_(new Impl()) {}

ScreenPresenter::~ScreenPresenter() {
  delete impl_;
}

void ScreenPresenter::begin() {
  impl_->begin();
}

void ScreenPresenter::showStatus(const char *line) {
  impl_->drawStatus(line);
}

void ScreenPresenter::show(const Frame &frame) {
  impl_->show(frame);
}

#endif
