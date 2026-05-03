#include "globals.h"
#include "tuner.h"

#define TUNER_TOUCH_DB 200
#define TUNER_DOUBLE_TAP_MS 350
static unsigned long tunerLastTouch = 0;

void tunerDraw() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("TUNER", 160, 120, 4);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("double-tap to return!", 160, 160, 2);
}

void tunerLoop() {
    unsigned long now = millis();
    if (now - tunerLastTouch >= TUNER_TOUCH_DB && ts.tirqTouched() && ts.touched()) {
        if (now - lastTapTime < TUNER_DOUBLE_TAP_MS) {
            currentMode = METRONOME;
            lastTapTime = 0;
        } else {
            lastTapTime = now;
        }
        tunerLastTouch = now;
    }
}