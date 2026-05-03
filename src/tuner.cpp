#include "globals.h"
#include "tuner.h"

void tunerDraw() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("TUNER", 160, 120, 4);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("double-tap to return!", 160, 160, 2);
}

void tunerLoop() {
    // placeholder for now
}