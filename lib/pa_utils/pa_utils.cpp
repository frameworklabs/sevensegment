// Copyright (c) 2022, Framework Labs.

#include "pa_utils.h"

#include <utility/Button.h>

// Timing

pa_activity_def (Delay, unsigned n) {
    pa_self.i = n;
    while (pa_self.i > 0) {
        pa_pause;
        pa_self.i -= 1;
    }
} pa_end;

// Button

pa_activity_def (ButtonUpdater, Button& button) {
    pa_always {
        button.read();
    } pa_always_end;
} pa_end;

pa_activity_def (DetectReleasePress, Button& button, bool& wasReleased, bool& wasPressed) {
    pa_await (button.wasReleased());
    wasReleased = true;
    pa_await (button.wasPressed());
    wasPressed = true;
} pa_end;

pa_activity_def (PressRecognizer, Button& button, Press& press) 
{
    while (true) {
        press = Press::NO;

        pa_await (button.wasPressed());

        pa_self.wasReleased = false;
        pa_self.wasPressed = false;
        
        pa_co(2) {
            pa_with_weak (Delay, 3);
            pa_with_weak (DetectReleasePress, button, pa_self.wasReleased, pa_self.wasPressed);
        } pa_co_end;

        if (pa_self.wasPressed) {
            press = Press::DOUBLE;
        } 
        else if (pa_self.wasReleased) {
            press = Press::SHORT;
        }
        else {
            press = Press::LONG;
        }

        pa_pause;
    }
} pa_end;

pa_activity_def (PressRecognizer2, Button& button1, Button& button2, Press& press) 
{
    while (true) {
        press = Press::NO;

        pa_await (button1.wasPressed() || button2.wasPressed());

        if (button2.wasPressed()) {
            press = Press::DOUBLE;
            pa_pause;
            continue;
        }

        pa_self.wasReleased = false;
        pa_self.wasPressed = false;
        
        pa_co(2) {
            pa_with_weak (Delay, 3);
            pa_with_weak (DetectReleasePress, button1, pa_self.wasReleased, pa_self.wasPressed);
        } pa_co_end;

        if (pa_self.wasPressed) {
            press = Press::DOUBLE;
        } 
        else if (pa_self.wasReleased) {
            press = Press::SHORT;
        }
        else {
            press = Press::LONG;
        }

        pa_pause;
    }
} pa_end;

pa_activity_def (PressInspector, std::string button, Press press) {
    pa_always {
        Serial.print(button.c_str());
        Serial.print(' ');
        switch (press) {
            case Press::NO: Serial.println("NO"); break;
            case Press::SHORT: Serial.println("SHORT"); break;
            case Press::LONG: Serial.println("LONG"); break;
            case Press::DOUBLE: Serial.println("DOUBLE"); break;
        }
    } pa_always_end;
} pa_end;

// Logical

pa_activity_def (RaisingEdgeDetector, bool val, bool& edge) {
    pa_self.prevVal = val;
    edge = val;
    pa_pause;

    pa_always {
        if (val != pa_self.prevVal) {
            pa_self.prevVal = val;
            edge = val;
        } else {
            edge = false;
        }
    } pa_always_end;
} pa_activity_end;
