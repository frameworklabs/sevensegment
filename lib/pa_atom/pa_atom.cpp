// Copyright (c) 2022, Framework Labs.

#include "pa_atom.h"

// LED

static CRGB mainLED;

void initLED() {
    FastLED.addLeds<NEOPIXEL, 27>(&mainLED, 1);
    FastLED.setBrightness(5);
}

void setLED(CRGB color) {
    mainLED = color;
    FastLED.show();
}

void clearLED() {
    setLED(CRGB::Black);
}

pa_activity_def (BlinkLED, CRGB color, unsigned on, unsigned off) {
    while (true) {
        setLED(color);
        pa_run (Delay, on);
        
        clearLED();
        pa_run (Delay, off);
    }
} pa_end;
