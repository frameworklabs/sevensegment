// pa_atom
//
// Copyright (c) 2022, Framework Labs.

#pragma once

#include "pa_atom_priv.h"

#include <FastLED.h>

// LED

void initLED();
void setLED(CRGB color);
void clearLED();

pa_activity_sig (BlinkLED, CRGB color, unsigned on, unsigned off);
