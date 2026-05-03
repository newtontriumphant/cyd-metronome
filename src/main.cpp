#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "tuner.h"
#include "globals.h"

Mode currentMode = METRONOME;
unsigned long lastTapTime = 0;
#define DOUBLE_TAP_MS 350

#define BUZZER_PIN 22
#define BUZZER_CH 0
#define BEEP_FREQ 1000
#define BEEP_MS 200
#define BPM_MIN 20
#define BPM_MAX 300

#define T_IRQ 36
#define T_MOSI 32
#define T_MISO 39
#define T_CLK 25
#define T_CS 33

int bpm = 100;
unsigned long lastBeat = 0;
unsigned long beepEnd = 0;
unsigned long flashEnd = 0;
bool beeping = false;
bool flashing = false;
bool running = true;
#define FLASH_MS 100

unsigned long lastTouch = 0;
#define TOUCH_DB 200

TFT_eSPI tft = TFT_eSPI();
SPIClass touchSPI(HSPI);
XPT2046_Touchscreen ts(T_CS, T_IRQ);

#define C_BG TFT_BLACK
#define C_FLASH TFT_RED
#define C_NUM TFT_GREEN
#define C_LBL TFT_LIGHTGREY
#define C_BTN 0x39C7
#define C_BTNT TFT_WHITE
#define C_LINE 0x4208

struct Btn { int16_t x, y, w, h, delta; const char* lbl; };
const Btn btns[4] = {
    {  4, 167, 74, 68, -10, "-10" },
    { 83, 167, 74, 68,  -1,  "-1" },
    {163, 167, 74, 68,  +1,  "+1" },
    {242, 167, 74, 68, +10, "+10" },
};

void drawBPMNumber() {
    tft.fillRect(40, 20, 240, 88, C_BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(C_NUM, C_BG);
    tft.setTextPadding(tft.textWidth("000", 7));
    tft.drawNumber(bpm, 160, 72, 7);
}

void drawTopArea() {
    tft.fillRect(0, 0, 320, 157, C_BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(running ? C_NUM : TFT_DARKGREY, C_BG);
    tft.setTextPadding(tft.textWidth("000", 7));
    tft.drawNumber(bpm, 160, 72, 7);
    tft.setTextColor(C_LBL, C_BG);
    tft.drawString(running ? "BPM" : "PAUSED", 160, 128, 4);
    tft.drawFastHLine(0, 157, 320, C_LINE); //divider
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("double-tap for tuner!", 160, 14, 1);
}

void drawUI(bool flash) {
    if (flash) {
        tft.fillRect(0, 0, 320, 155, C_FLASH);
        return;
    }
    
    tft.setTextPadding(0);
    tft.fillScreen(C_BG);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(C_NUM, C_BG);
    tft.drawNumber(bpm, 160, 72, 7);

    tft.setTextColor(C_LBL, C_BG);
    tft.drawString("BPM", 160, 128, 4);
    tft.drawFastHLine(0, 157, 320, C_LINE); //divider

    for (int i = 0; i < 4; i++) {
        const Btn& b = btns[i];
        tft.fillRoundRect(b.x, b.y, b.w, b.h, 8, C_BTN);
        tft.setTextColor(C_BTNT, C_BTN);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(b.lbl, b.x + b.w / 2, b.y + b.h / 2, 4);
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(21, OUTPUT);
    digitalWrite(21, HIGH);
    pinMode(BUZZER_PIN, OUTPUT);

    touchSPI.begin(T_CLK, T_MISO, T_MOSI, -1);
    ts.begin(touchSPI);
    ts.setRotation(1);
    tft.init();
    tft.setRotation(1);
    drawUI(false);
    lastBeat = millis();
}

void loop() {
    if (currentMode == TUNER) {
        tunerLoop();

        if (currentMode == METRONOME) {
            flashing = false;
            beeping = false;
            digitalWrite(BUZZER_PIN, LOW);
            running = true;
            lastBeat = millis();
            drawUI(false);
        }

        return;
    }

    unsigned long now = millis();
    unsigned long interval = 60000UL / (unsigned long)bpm;

    if (running && now - lastBeat >= interval) {
        lastBeat += interval;
        digitalWrite(BUZZER_PIN, HIGH);
        beepEnd = now + BEEP_MS;
        beeping = true;

        flashing = true;
        flashEnd = now + FLASH_MS;
        drawUI(true);
    }

    if (beeping && now >= beepEnd) {
        digitalWrite(BUZZER_PIN, LOW);
        beeping = false;
    }
    if (flashing && now >= flashEnd) {
        flashing = false;
        drawTopArea();
    }

    if (now - lastTouch >= TOUCH_DB && ts.tirqTouched() && ts.touched()) {
        TS_Point p = ts.getPoint();
        int tx = map(p.x, 200, 3700, 0, 320);
        int ty = map(p.y, 240, 3800, 0, 240);

        if (now - lastTapTime < DOUBLE_TAP_MS) {
            currentMode = (currentMode == METRONOME) ? TUNER : METRONOME;
            if (currentMode == TUNER) tunerDraw();
            else { running = true; lastBeat = millis(); drawUI(false); }
            lastTouch = now;
            lastTapTime = 0;
        } else {
            lastTapTime = now;
            if (ty < 157) {
                running = !running;
                if (running) lastBeat = millis();
                drawTopArea();
                lastTouch = now;
            } else {
                for (int i = 0; i < 4; i++) {
                    const Btn& b = btns[i];
                    if (tx >= b.x && tx < b.x + b.w && ty >= b.y && ty < b.y + b.h) {
                        bpm = constrain(bpm + b.delta, BPM_MIN, BPM_MAX);
                        if (!flashing) drawBPMNumber();
                        lastTouch = now;
                        break;
                    }
                }
            }
        }
    }
}