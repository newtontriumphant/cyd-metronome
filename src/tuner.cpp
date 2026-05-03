#include "globals.h"
#include "tuner.h"
#include <arduinoFFT.h>

#define TUNER_TOUCH_DB 200
#define TUNER_DOUBLE_TAP_MS 350
static unsigned long tunerLastTouch = 0;

#define MIC_PIN 34
#define SAMPLES 512
#define SAMPLE_RATE 8000
#define NOISE_THRESH 80

double vReal[SAMPLES];
double vImag[SAMPLES];
ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLE_RATE);

const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
float freqToMidi(float f) { return 12.0f * log2f(f / 440.0f) + 69.0f; }
float midiToFreq(int m) { return 440.0f * powf(2.0f, (m - 69.0f) / 12.0f); }

static void drawResult(float freq, float cents, const char* note, int octave) {
    tft.fillRect(0, 26, 320, 191, TFT_BLACK);
    char buf[8];
    sprintf(buf, "%s%d", note, octave);
    tft.setTextDatum(MC_DATUM);
    tft.setTextPadding(tft.textWidth("B#0", 7));
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(buf, 160, 80, 7);
    tft.setTextPadding(0);

    char freqBuf[20];
    sprintf(freqBuf, "%.1f Hz", freq);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString(freqBuf, 160, 128, 4);

    int barY = 50;
    tft.fillRect(60, barY, 200, 18, 0x1082);
    tft.drawFastVLine(160, barY - 4, 26, TFT_DARKGREY);
    uint16_t col = (abs(cents) < 5) ? TFT_GREEN : (abs(cents) < 15) ? TFT_YELLOW : TFT_RED;
    int px = constrain((int)(cents * 2.0f), -100, 100);
    if (px > 0) tft.fillRect(160, barY + 2, px, 14, col);
    else if (px < 0) tft.fillRect(160 + px, barY + 2, -px, 14, col);

    char centsBuf[10];
    sprintf(centsBuf, "%+.0f cents", cents);
    tft.setTextColor(col, TFT_BLACK);
    tft.drawString(centsBuf, 160, 185, 2);
}

static void drawListening() {
    tft.fillRect(0, 26, 320, 191, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("---", 160, 80, 7);
    tft.drawString("listening...", 160, 128, 2);
}

void tunerDraw() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("TUNER", 160, 14, 2);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("double-tap to return!", 160, 228, 1);
    tft.drawFastHLine(0, 25, 320, 0x2104);
    tft.drawFastHLine(0, 218, 320, 0x2104);
    drawListening();
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
        return;
    }
    analogSetPinAttenuation(MIC_PIN, ADC_11db);
    for(int i = 0; i < SAMPLES; i++) {
        vReal[i] = analogRead(MIC_PIN) - 2048.0;
        vImag[i] = 0;
        delayMicroseconds(125); // generates a ~8000Hz sample rate
    }

    double peak = 0;
    for (int i = 0; i < SAMPLES; i++) if (abs(vReal[i]) > peak) peak = abs(vReal[i]);
    if (peak < NOISE_THRESH) { drawListening(); return; }

    FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    FFT.compute(FFTDirection::Forward);
    FFT.complexToMagnitude();

    double peakFreq = FFT.majorPeak();
    if (peakFreq < 50 || peakFreq > 2000) { drawListening(); return; }
    float midi = freqToMidi((float)peakFreq);
    int midiNote = (int)round(midi);
    int noteIdx = ((midiNote % 12) + 12) % 12;
    int octave = (midiNote / 12) - 1;
    float refFreq = midiToFreq(midiNote);
    float cents = 1200.0f * log2f((float)peakFreq / refFreq);

    drawResult((float)peakFreq, cents, noteNames[noteIdx], octave);
}