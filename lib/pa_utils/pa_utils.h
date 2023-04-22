// pa_utils
//
// Copyright (c) 2022, Framework Labs.

#pragma once

#include "pa_utils_priv.h"

#include <string>

// Timing

pa_activity_sig (Delay, unsigned n);

// Button

class Button;

enum class Press : uint8_t {
    NO,
    SHORT,
    DOUBLE,
    LONG
};

pa_activity_sig (ButtonUpdater, Button& button);

pa_activity_sig (PressRecognizer, Button& button, Press& press);

pa_activity_sig (PressRecognizer2, Button& button1, Button& button2, Press& press);

pa_activity_sig (PressInspector, std::string button, Press press);

// Logical

pa_activity_sig (RaisingEdgeDetector, bool val, bool& edge);
