#include "adapters/screen_presenter.h"

#if APP_MODE == MODE_SCREEN

#include "adapters/backlight.h"
#include "domain/weather.h"

#include <Arduino.h>
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
const int PAGE_HOME = 0;
const int PAGE_SETTINGS = 1;
const int PAGE_BRIGHTNESS = 2;
const int GEAR_CX = 298;
const int GEAR_CY = 220;
const int BTN_X = 20;
const int BTN_W = 280;
const int BTN_H = 62;
const int BTN_HOME_Y = 62;
const int BTN_LIGHT_Y = 142;
const int BTN_AUTO_Y = 146;
const int STEP_Y = 56;
const int STEP_S = 72;
const int MINUS_X = 24;
const int PLUS_X = 224;
const int HIT_NONE = 0;
const int HIT_GEAR = 1;
const int HIT_HOME = 2;
const int HIT_LIGHT = 3;
const int HIT_AUTO = 4;
const int HIT_BACK = 5;
const int HIT_MINUS = 6;
const int HIT_PLUS = 7;
const unsigned long STEP_HOLD_MS = 380;
const unsigned long STEP_REPEAT_MS = 130;
const int MARK_GRAVE = 1;
const int MARK_ACUTE = 2;
const int MARK_CIRC = 3;
const int MARK_CEDILLA = 4;

bool inside(int x, int y, int left, int top, int width, int height) {
  return x >= left && y >= top && x < left + width && y < top + height;
}

bool nextUtf8(const char *&text, char *base, int *mark) {
  if (*text == '\0') return false;
  unsigned char c = (unsigned char)*text;
  if (c < 0x80) {
    *base = (char)c;
    *mark = 0;
    text++;
    return true;
  }
  if (c == 0xC3 && text[1] != '\0') {
    unsigned char n = (unsigned char)text[1];
    text += 2;
    *mark = 0;
    *base = '?';
    if (n == 0xA0) { *base = 'a'; *mark = MARK_GRAVE; }
    else if (n == 0xA2) { *base = 'a'; *mark = MARK_CIRC; }
    else if (n == 0xA7) { *base = 'c'; *mark = MARK_CEDILLA; }
    else if (n == 0xA8) { *base = 'e'; *mark = MARK_GRAVE; }
    else if (n == 0xA9) { *base = 'e'; *mark = MARK_ACUTE; }
    else if (n == 0xAA) { *base = 'e'; *mark = MARK_CIRC; }
    else if (n == 0xB4) { *base = 'o'; *mark = MARK_CIRC; }
    else if (n == 0xB9) { *base = 'u'; *mark = MARK_GRAVE; }
    else if (n == 0xBB) { *base = 'u'; *mark = MARK_CIRC; }
    else if (n == 0x80) { *base = 'A'; *mark = MARK_GRAVE; }
    else if (n == 0x88) { *base = 'E'; *mark = MARK_GRAVE; }
    else if (n == 0x89) { *base = 'E'; *mark = MARK_ACUTE; }
    else if (n == 0x8A) { *base = 'E'; *mark = MARK_CIRC; }
    return true;
  }
  text++;
  *base = '?';
  *mark = 0;
  return true;
}

void toPlain(const char *text, char *out, size_t size) {
  size_t n = 0;
  char base = 0;
  int mark = 0;
  while (n + 1 < size && nextUtf8(text, &base, &mark)) out[n++] = base;
  out[n] = '\0';
}

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
  Backlight backlight_;
  Frame lastFrame_{};
  bool hasFrame_ = false;
  bool covering_ = false;
  int page_ = PAGE_HOME;
  bool finger_ = false;
  int held_ = HIT_NONE;
  bool heldLit_ = false;
  bool stepDirty_ = false;
  unsigned long stepNextAt_ = 0;

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

  void drawMark(int mid, int top, int mark, uint16_t color) {
    if (mark == MARK_ACUTE) {
      tft.drawLine(mid - 3, top + 3, mid + 2, top, color);
      tft.drawLine(mid - 2, top + 4, mid + 3, top + 1, color);
    } else if (mark == MARK_GRAVE) {
      tft.drawLine(mid - 2, top, mid + 3, top + 3, color);
      tft.drawLine(mid - 1, top + 1, mid + 4, top + 4, color);
    } else if (mark == MARK_CIRC) {
      tft.drawLine(mid - 3, top + 3, mid, top, color);
      tft.drawLine(mid, top, mid + 3, top + 3, color);
      tft.drawLine(mid - 3, top + 4, mid, top + 1, color);
      tft.drawLine(mid, top + 1, mid + 3, top + 4, color);
    } else if (mark == MARK_CEDILLA) {
      tft.drawLine(mid - 1, top, mid + 1, top + 3, color);
    }
  }

  void printUtf8(const char *text, uint16_t color) {
    const char *cursor = text;
    char base = 0;
    int mark = 0;
    while (nextUtf8(cursor, &base, &mark)) {
      int16_t x0 = tft.getCursorX();
      int16_t y0 = tft.getCursorY();
      char letter[2] = {base, '\0'};
      tft.print(letter);
      if (!mark) continue;
      int16_t bx, by;
      uint16_t bw, bh;
      tft.getTextBounds(letter, x0, y0, &bx, &by, &bw, &bh);
      int roof = (int)by - 3;
      if (mark == MARK_CEDILLA) roof = (int)by + (int)bh;
      drawMark(bx + (int)bw / 2, roof, mark, color);
    }
  }

  void drawCentered(const char *text, int bandY, int bandH) {
    char plain[80];
    toPlain(text, plain, sizeof(plain));
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(plain, 0, 0, &x1, &y1, &w, &h);
    int top = bandY + (bandH - (int)h) / 2;
    tft.setCursor((320 - (int)w) / 2 - x1, top - y1);
    printUtf8(text, ink());
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
    covering_ = true;
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
    backlight_.begin();
  }

  void drawWeather(const Frame &frame) {
    tft.fillRect(0, BAND_WEATHER_Y, 320, BAND_WEATHER_H, bg());
    tft.setFont(&GoogleSansRegular15);
    tft.setTextSize(1);
    tft.setTextColor(ink());
    if (!frame.weatherReady) {
      drawCentered("Météo...", BAND_WEATHER_Y, BAND_WEATHER_H);
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
    char plainRest[40];
    toPlain(rest, plainRest, sizeof(plainRest));
    tft.getTextBounds(temp, 0, 0, &tx, &ty, &tw, &th);
    tft.getTextBounds(plainRest, 0, 0, &rx, &ry, &rw, &rh);

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
    printUtf8(rest, ink());
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

  uint16_t trackColor() const { return night ? 0x39E7 : 0xC618; }

  void drawGear(bool pressed) {
    uint16_t color = pressed ? bg() : ink();
    tft.fillCircle(GEAR_CX, GEAR_CY, 15, pressed ? ink() : bg());
    tft.drawCircle(GEAR_CX, GEAR_CY, 14, color);
    tft.drawCircle(GEAR_CX, GEAR_CY, 5, color);
    tft.fillCircle(GEAR_CX, GEAR_CY, 2, color);
    const float pi = 3.1415926f;
    for (int i = 0; i < 8; i++) {
      float angle = i * pi / 4.0f;
      int outer = 11;
      int inner = 7;
      int x0 = GEAR_CX + (int)lroundf(cosf(angle) * inner);
      int y0 = GEAR_CY + (int)lroundf(sinf(angle) * inner);
      int x1 = GEAR_CX + (int)lroundf(cosf(angle) * outer);
      int y1 = GEAR_CY + (int)lroundf(sinf(angle) * outer);
      tft.drawLine(x0, y0, x1, y1, color);
    }
  }

  void drawSun(int x, int y, uint16_t color) {
    tft.drawCircle(x + 12, y + 12, 5, color);
    tft.drawLine(x + 12, y + 1, x + 12, y + 4, color);
    tft.drawLine(x + 12, y + 20, x + 12, y + 23, color);
    tft.drawLine(x + 1, y + 12, x + 4, y + 12, color);
    tft.drawLine(x + 20, y + 12, x + 23, y + 12, color);
    tft.drawLine(x + 4, y + 4, x + 6, y + 6, color);
    tft.drawLine(x + 18, y + 6, x + 20, y + 4, color);
    tft.drawLine(x + 4, y + 20, x + 6, y + 18, color);
    tft.drawLine(x + 18, y + 18, x + 20, y + 20, color);
  }

  void drawClockIcon(int x, int y, uint16_t color) {
    tft.drawCircle(x + 12, y + 12, 9, color);
    tft.drawLine(x + 12, y + 12, x + 12, y + 6, color);
    tft.drawLine(x + 12, y + 12, x + 17, y + 15, color);
  }

  void drawBack() {
    uint16_t color = ink();
    tft.drawLine(18, 16, 8, 26, color);
    tft.drawLine(8, 26, 18, 36, color);
  }

  void printTextAt(const char *text, int x, int top, int height, uint16_t color) {
    char plain[40];
    toPlain(text, plain, sizeof(plain));
    tft.setFont(&GoogleSansRegular15);
    tft.setTextSize(1);
    tft.setTextColor(color);
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(plain, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor(x - x1, top + (height - (int)h) / 2 - y1);
    printUtf8(text, color);
    (void)w;
  }

  void drawMenuButton(int top, const char *label, bool sun, bool pressed) {
    uint16_t fill = pressed ? ink() : bg();
    uint16_t mark = pressed ? bg() : ink();
    tft.fillRoundRect(BTN_X, top, BTN_W, BTN_H, 12, fill);
    tft.drawRoundRect(BTN_X, top, BTN_W, BTN_H, 12, ink());
    if (sun) drawSun(BTN_X + 16, top + 18, mark);
    else drawClockIcon(BTN_X + 16, top + 18, mark);
    printTextAt(label, BTN_X + 56, top, BTN_H, mark);
  }

  void drawSettings() {
    tft.fillScreen(bg());
    tft.setFont(&GoogleSansRegular15);
    tft.setTextSize(1);
    tft.setTextColor(ink());
    drawCentered("Réglages", 8, 40);
    drawMenuButton(BTN_HOME_Y, "Accueil", false, false);
    drawMenuButton(BTN_LIGHT_Y, "Luminosité", true, false);
  }

  void drawStepButton(int x, const char *label, bool pressed) {
    uint16_t fill = pressed ? ink() : bg();
    uint16_t mark = pressed ? bg() : ink();
    tft.fillRoundRect(x, STEP_Y, STEP_S, STEP_S, 12, fill);
    tft.drawRoundRect(x, STEP_Y, STEP_S, STEP_S, 12, ink());
    tft.setFont(&GoogleSansRegular15);
    tft.setTextSize(2);
    tft.setTextColor(mark);
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(label, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor(x + (STEP_S - (int)w) / 2 - x1, STEP_Y + (STEP_S - (int)h) / 2 - y1);
    tft.print(label);
    tft.setTextSize(1);
  }

  void drawPercent() {
    int left = MINUS_X + STEP_S;
    int width = PLUS_X - left;
    tft.fillRect(left, STEP_Y, width, STEP_S, bg());
    char label[8];
    snprintf(label, sizeof(label), "%d%%", backlight_.percent());
    tft.setFont(&GoogleSansRegular15);
    tft.setTextSize(2);
    tft.setTextColor(ink());
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(label, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor(left + (width - (int)w) / 2 - x1, STEP_Y + (STEP_S - (int)h) / 2 - y1);
    tft.print(label);
    tft.setTextSize(1);
  }

  void drawAuto(bool pressed) {
    bool on = backlight_.automatic();
    uint16_t fill = bg();
    uint16_t mark = ink();
    if (pressed) fill = trackColor();
    else if (on) {
      fill = ink();
      mark = bg();
    }
    tft.fillRoundRect(BTN_X, BTN_AUTO_Y, BTN_W, BTN_H, 12, fill);
    tft.drawRoundRect(BTN_X, BTN_AUTO_Y, BTN_W, BTN_H, 12, ink());
    drawSun(BTN_X + 16, BTN_AUTO_Y + 18, mark);
    printTextAt("Auto", BTN_X + 56, BTN_AUTO_Y, BTN_H, mark);
    const char *state = on ? "ON" : "OFF";
    int16_t x1, y1;
    uint16_t w, h;
    tft.setFont(&GoogleSansRegular15);
    tft.setTextSize(1);
    tft.getTextBounds(state, 0, 0, &x1, &y1, &w, &h);
    printTextAt(state, BTN_X + BTN_W - 18 - (int)w, BTN_AUTO_Y, BTN_H, mark);
  }

  void drawBrightness() {
    tft.fillScreen(bg());
    drawBack();
    tft.setFont(&GoogleSansRegular15);
    tft.setTextSize(1);
    tft.setTextColor(ink());
    drawCentered("Luminosité", 8, 40);
    drawStepButton(MINUS_X, "-", false);
    drawPercent();
    drawStepButton(PLUS_X, "+", false);
    drawAuto(false);
  }

  void resetHome() {
    painted = false;
    weatherPainted = false;
    datePainted[0] = '\0';
    clockPainted[0] = '\0';
  }

  void open(int page) {
    held_ = HIT_NONE;
    heldLit_ = false;
    page_ = page;
    if (page == PAGE_HOME) {
      resetHome();
      if (hasFrame_) show(lastFrame_);
      return;
    }
    if (page == PAGE_SETTINGS) drawSettings();
    else drawBrightness();
  }

  int hitAt(int x, int y) const {
    if (page_ == PAGE_HOME) {
      if (inside(x, y, GEAR_CX - 22, GEAR_CY - 22, 44, 44)) return HIT_GEAR;
      return HIT_NONE;
    }
    if (page_ == PAGE_SETTINGS) {
      if (inside(x, y, BTN_X, BTN_HOME_Y, BTN_W, BTN_H)) return HIT_HOME;
      if (inside(x, y, BTN_X, BTN_LIGHT_Y, BTN_W, BTN_H)) return HIT_LIGHT;
      return HIT_NONE;
    }
    if (y < 48) return HIT_BACK;
    if (inside(x, y, MINUS_X, STEP_Y, STEP_S, STEP_S)) return HIT_MINUS;
    if (inside(x, y, PLUS_X, STEP_Y, STEP_S, STEP_S)) return HIT_PLUS;
    if (inside(x, y, BTN_X, BTN_AUTO_Y, BTN_W, BTN_H)) return HIT_AUTO;
    return HIT_NONE;
  }

  void paintHeld(bool lit) {
    if (held_ == HIT_NONE || lit == heldLit_) return;
    heldLit_ = lit;
    if (held_ == HIT_GEAR) drawGear(lit);
    else if (held_ == HIT_HOME) drawMenuButton(BTN_HOME_Y, "Accueil", false, lit);
    else if (held_ == HIT_LIGHT) drawMenuButton(BTN_LIGHT_Y, "Luminosité", true, lit);
    else if (held_ == HIT_MINUS) drawStepButton(MINUS_X, "-", lit);
    else if (held_ == HIT_PLUS) drawStepButton(PLUS_X, "+", lit);
    else if (held_ == HIT_AUTO) drawAuto(lit);
    else if (held_ == HIT_BACK) {
      uint16_t color = lit ? bg() : ink();
      tft.fillCircle(14, 26, 16, lit ? ink() : bg());
      tft.drawLine(18, 16, 8, 26, color);
      tft.drawLine(8, 26, 18, 36, color);
    }
  }

  bool isStep(int hit) const {
    return hit == HIT_MINUS || hit == HIT_PLUS;
  }

  void applyStep(int hit) {
    int before = backlight_.percent();
    bool wasAuto = backlight_.automatic();
    backlight_.nudge(hit == HIT_PLUS ? 5 : -5);
    if (backlight_.percent() == before && !wasAuto) return;
    drawPercent();
    if (wasAuto) drawAuto(false);
    stepDirty_ = true;
  }

  void activate(int hit) {
    if (hit == HIT_GEAR || hit == HIT_BACK) open(PAGE_SETTINGS);
    else if (hit == HIT_HOME) open(PAGE_HOME);
    else if (hit == HIT_LIGHT) open(PAGE_BRIGHTNESS);
    else if (hit == HIT_AUTO) {
      backlight_.setAutomatic(!backlight_.automatic());
      backlight_.save();
      drawPercent();
      drawAuto(false);
      held_ = HIT_NONE;
      heldLit_ = false;
    }
  }

  void interact(const TouchSample &sample) {
    if (!hasFrame_ || covering_) return;

    if (sample.down && !finger_) {
      finger_ = true;
      held_ = hitAt(sample.x, sample.y);
      heldLit_ = false;
      paintHeld(true);
      if (isStep(held_)) {
        applyStep(held_);
        stepNextAt_ = millis() + STEP_HOLD_MS;
      }
      return;
    }

    if (sample.down && finger_) {
      bool over = hitAt(sample.x, sample.y) == held_;
      paintHeld(over);
      if (over && isStep(held_) && (long)(millis() - stepNextAt_) >= 0) {
        applyStep(held_);
        stepNextAt_ = millis() + STEP_REPEAT_MS;
      }
      return;
    }

    if (!sample.down && finger_) {
      if (!isStep(held_) && held_ != HIT_NONE && hitAt(sample.x, sample.y) == held_) activate(held_);
      else if (heldLit_) paintHeld(false);
      if (stepDirty_) {
        backlight_.save();
        stepDirty_ = false;
      }
      held_ = HIT_NONE;
      heldLit_ = false;
      finger_ = false;
    }
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
    lastFrame_ = frame;
    hasFrame_ = true;
    covering_ = false;
    night = frame.night;
    int shown = backlight_.percent();
    backlight_.followNight(frame.night);

    if (page_ != PAGE_HOME) {
      if (frame.themeChanged) {
        if (page_ == PAGE_SETTINGS) drawSettings();
        else drawBrightness();
      } else if (page_ == PAGE_BRIGHTNESS && backlight_.percent() != shown) {
        drawPercent();
      }
      return;
    }

    if (frame.themeChanged) resetHome();

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
    if (held_ != HIT_GEAR) drawGear(false);
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

void ScreenPresenter::interact(const TouchSample &sample) {
  impl_->interact(sample);
}

#endif
