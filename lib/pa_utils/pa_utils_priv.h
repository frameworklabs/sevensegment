// Copyright (c) 2022, Framework Labs.

#pragma once

#include <proto_activities.h>

// Timing

pa_activity_ctx (Delay, unsigned i);

// Button

class Button;

pa_activity_ctx (ButtonUpdater);

pa_activity_decl (DetectReleasePress, pa_ctx(), Button& button, bool& wasReleased, bool& wasPressed);

pa_activity_ctx (PressRecognizer, bool wasPressed; bool wasReleased; pa_co_res(2); pa_use(Delay); pa_use(DetectReleasePress));

pa_activity_ctx (PressRecognizer2, bool wasPressed; bool wasReleased; pa_co_res(2); pa_use(Delay); pa_use(DetectReleasePress));

pa_activity_ctx (PressInspector);

// Logical

pa_activity_ctx (RaisingEdgeDetector, bool prevVal);
