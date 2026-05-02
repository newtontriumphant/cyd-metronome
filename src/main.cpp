#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

#define BUZZER_PIN 26
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
#define FLASH_MS 60

unsigned long lastTouch = 0;
#define TOUCH_DB 200

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);
SPIClass touchSPI(VSPI);
XPT2046_Touchscreen ts(T_CS, T_IRQ);

#define C_BG TFT_BLACK
#define C_FLASH TFT_RED
#define C_NUM TFT_GREEN
#define C_LBL TFT_LIGHTGREY
#define C_BTN 0x39C7   // medium-dark grey
#define C_BTNT TFT_WHITE
#define C_LINE 0x4208

struct Btn { int16_t x, y, w, h, delta; const char* lbl; };
const Btn btns[4] = {
  {  4, 167, 74, 68, -10, "-10" },
  { 83, 167, 74, 68,  -1,  "-1" },
  {163, 167, 74, 68,  +1,  "+1" },
  {242, 167, 74, 68, +10, "+10" },
};

void drawUI(bool, flash) {
    spr.fillSprite(flash ? C_FLASH : C_BG);

    if (!flash) {
        spr.setTextDatum(MC_DATUM);
        spr.setTextColor(C_NUM, C_BG);
        spr.drawNumber(bpm, 160, 72, 7);

        spr.setTextColor(C_LBL, C_BG);
        spr.drawString("BPM", 160, 128, 4);
        spr.drawFastHLine(0, 157, 320, C_LINE); //divider

        for (int i = 0; i < 4; i++) {
            const Btn& b = btns[i];
            spr.fillRoundRect(b.x, b.y, b.w, b.h, 8, C_BTN);
            spr.setTextColor(C_BTNT, C_BTN);
            spr.setTextDatum(MC_DATUM);
            spr.drawString(b.lbl, b.x + b.w / 2, b.y + b.h / 2, 4);
        }
    }

    spr.pushSprite(0, 0);
}

void setup() {
    Serial.begin(115200);
    ledcSetup(BUZZER_CH, BEEP_FREQ, 8);
    ledcAttachPin(BUZZER_PIN, BUZZER_CH);
    ledcWrite(BUZZER_CH, 0);

    touchSPI.begin(T_CLK, T_MISO, T_MOSI, T_CS);
    ts.begin(touchSPI);
    ts.setRotation(0);

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(C_BG);
    spr.createSprite(320, 240);
    drawUI(false);
    lastBeat = millis();
}

void loop() {}