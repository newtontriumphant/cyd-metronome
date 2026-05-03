#pragma once
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
extern TFT_eSPI tft;
extern XPT2046_Touchscreen ts;

enum Mode { METRONOME, TUNER };
extern Mode currentMode; 
extern unsigned long lastTapTime;