// sevensegment
//
// Copyright (c) 2023, Framework Labs.

#include <pa_atom.h>
#include <pa_utils.h>
#include <proto_activities.h>

#include <M5Atom.h>

#include <cstdio>

// Input helpers

static bool is_any_press(Press press) {
    return press != Press::NO;
}

static bool is_long_or_double_press(Press press) {
    return press == Press::LONG || press == Press::DOUBLE;
}

// Led

enum Led : uint8_t {
    top = 0b00000010,
    top_right = 0b00000100,
    middle = 0b00001000,
    bottom_right = 0b00010000,
    bottom = 0b00100000,
    bottom_left = 0b00000001,
    top_left = 0b01000000,
};

static constexpr Led led_blank = Led(0);
static constexpr Led led_mult = Led(Led::top_left | Led::top_right | Led::middle | Led::bottom_left | Led::bottom_right);
static constexpr Led led_div = Led(Led::top_right | Led::middle | Led::bottom_left);
static constexpr Led led_plus = Led(Led::top | Led::top_left | Led::top_right | Led::middle);
static constexpr Led led_minus = Led::middle;
static constexpr Led led_decimal = Led::bottom;
static constexpr Led led_equal = Led(Led::top | Led::bottom);
static constexpr Led led_e = Led(Led::top | Led::top_left | Led::middle | Led::bottom_left | Led::bottom);
static constexpr Led led_r = Led(Led::top | Led::top_left | Led::top_right | Led::middle | Led::bottom_left | Led::bottom_right);

static Led led_digit(uint8_t digit) {
        static uint8_t segments[] = {
            Led::top | Led::top_right | Led::bottom_right | Led::bottom | Led::bottom_left | Led::top_left,
            Led::top_right | Led::bottom_right,
            Led::top | Led::top_right | Led::middle | Led::bottom_left | Led::bottom,
            Led::top | Led::top_right | Led::middle | Led::bottom_right | Led::bottom,
            Led::top_left | Led::top_right | Led::middle | Led::bottom_right,
            Led::top | Led::top_left | Led::middle | Led::bottom_right | Led::bottom,
            Led::top | Led::top_left | Led::middle | Led::bottom_left | Led::bottom_right | Led::bottom,
            Led::top | Led::top_right | Led::bottom_right,
            Led::top | Led::top_right | Led::middle | Led::bottom_right | Led::bottom | Led::bottom_left | Led::top_left,
            Led::top | Led::top_right | Led::middle | Led::bottom_right | Led::bottom | Led::top_left,
        };
        if (digit > 9) {
            return led_blank;
        }
        return Led(segments[digit]);
}

// Starts with 0 at top and grows clock wise up to 5.
static Led led_circle(uint8_t pos) {
    static Led segments[] = {
        Led::top,
        Led::top_right,
        Led::bottom_right,
        Led::bottom,
        Led::bottom_left,
        Led::top_left,
    };
    if (pos > 5) {
        return led_blank;
    }   
    return segments[pos];
}

static Led led_error(uint8_t pos) {
    static Led segments[] = {
        led_e, led_r, led_r, led_digit(0), led_r
    };
    if (pos > 4) {
        return led_blank;
    }   
    return segments[pos];
}

class LedPort final {
public:
    void begin(uint8_t ds_pin, uint8_t sh_cp_pin, uint8_t st_cp_pin) {
        ds_pin_ = ds_pin;
        sh_cp_pin_ = sh_cp_pin;
        st_cp_pin_ = st_cp_pin;
        pinMode(ds_pin_, OUTPUT);
        pinMode(sh_cp_pin_, OUTPUT);
        pinMode(st_cp_pin_, OUTPUT);      
    }

    void write(Led led) {
        digitalWrite(st_cp_pin_, LOW);
        shiftOut(ds_pin_, sh_cp_pin_, MSBFIRST, led);
        digitalWrite(st_cp_pin_, HIGH);        
    }

private:
    uint8_t ds_pin_, sh_cp_pin_, st_cp_pin_;
};

static LedPort ledPort;

// Calculator abstractions

enum class Op {
    add,
    sub,
    mult,
    div,
};

enum EntryMode : uint8_t {
    sign = 0b0001,
    decimal = 0b0010,
    zero = 0b0100,
};

struct Entry {
    enum class Type {
        digit,
        sign,
        decimal,
    };

    Type type;
    uint8_t digit;

    static Entry makeDigit(uint8_t digit) {
        return Entry{Type::digit, digit};
    }
    static Entry makeSign() {
        return Entry{Type::sign};
    }
    static Entry makeDecimal() {
        return Entry{Type::decimal};
    }
};

class NumberModel {
public:
    void reset() {
        *this = {};
    }
    EntryMode allowedEntryMode() const {
        uint8_t mode = 0;
        if (!(isSigned || hasDigit)) {
            mode |= EntryMode::sign;
        }
        if (!hasDecimal) {
            mode |= EntryMode::decimal;
        }
        if (!hasLeadingZero) {
            mode |= EntryMode::zero;
        }
        return EntryMode(mode);
    }

    void update(Entry entry) {
        switch (entry.type) {
        case Entry::Type::sign:
            isSigned = true;
            text.push_back('-');
            break;

        case Entry::Type::decimal:
            hasDecimal = true;
            text.push_back('.');
            break;

        case Entry::Type::digit:
            if (entry.digit == 0) {
                if (!hasDigit) {
                    hasLeadingZero = true;
                } else {
                    hasDigit = true;
                }
            }
            text.push_back('0' + entry.digit);
            break;
        }
    }

    bool hasError() const {
        char* end = nullptr;
        auto f = strtof(text.c_str(), &end);
        if (end == text.c_str()) {
            return true;
        }
        return errno == ERANGE;
    }

    float getValue() const {
        char* end = nullptr;
        return strtof(text.c_str(), &end);
    }

private:
    bool isSigned = false;
    bool hasDigit = false;
    bool hasDecimal = false;
    bool hasLeadingZero = false;
    std::string text;
};

class Calculator {
public:
    void calculate (const NumberModel& lhs, const Op op, const NumberModel& rhs) {
        has_error_ = lhs.hasError() || rhs.hasError();
        if (has_error_) {
            return;
        }
        switch (op) {
            case Op::add: value_ = lhs.getValue() + rhs.getValue(); return;
            case Op::sub: value_ = lhs.getValue() - rhs.getValue(); return;
            case Op::mult: value_ = lhs.getValue() * rhs.getValue(); return;
            case Op::div:
                if (rhs.getValue() == 0.0) {
                    has_error_ = true;
                    return;
                }
                value_ = lhs.getValue() / rhs.getValue(); 
                return;
        }
    }

    bool hasError() const { return has_error_; }
    float getValue() const { return value_; }

    std::string getFormattedValue() {
        char buf[8];
        const auto num = snprintf(buf, sizeof(buf), "%f", value_);
        if (num < 0) {
            return {};
        } else {
            auto res = std::string(buf);
            while (res.size() > 1) {
                const auto c = res.back();
                if (c == '0') {
                    res.pop_back();
                    continue;
                }
                if (c == '.') {
                    res.pop_back();
                    break;
                }
                break;
            }
            return res;
        }
    }

private:
    bool has_error_;
    float value_;
};

// Activities

static constexpr unsigned presentation_delay_short = 2;
static constexpr unsigned presentation_delay_medium = 8;
static constexpr unsigned presentation_delay_long = 12;

pa_activity (StartAnimator, pa_ctx(pa_use(Delay); uint8_t pos), Led& led) {
    while (true) {
        led = led_circle(pa_self.pos);
        pa_self.pos = (pa_self.pos + 1) % 6;
        pa_run (Delay, presentation_delay_short);
    }
} pa_end;

pa_activity (OpSelector, pa_ctx(), Press press, Op& op, Led& led) {
    op = Op::add;

    while (true) {
        switch (op) {
            case Op::add: led = led_plus; break;
            case Op::sub: led = led_minus; break;
            case Op::mult: led = led_mult; break;
            case Op::div: led = led_div; break;
        }
        pa_await (press == Press::SHORT);
        op = Op((int(op) + 1) % 4);
    }
} pa_end;

pa_activity (EntrySelector, pa_ctx(uint8_t digit), Press press, EntryMode mode, Entry& entry, Led& led) {
    while (true) {
        if (mode & EntryMode::zero) {
            pa_self.digit = 0;
        } else {
            pa_self.digit = 1;
        }
        while (pa_self.digit <= 9) {
            entry = Entry::makeDigit(pa_self.digit);
            led = led_digit(pa_self.digit);
            pa_await (press == Press::SHORT);
            ++pa_self.digit;
        }
        if (mode & EntryMode::sign) {
            entry = Entry::makeSign();
            led = led_minus;
            pa_await (press == Press::SHORT);
        }
        if (mode & EntryMode::decimal) {
            entry = Entry::makeDecimal();
            led = led_decimal;
            pa_await (press == Press::SHORT);
        }
    }
} pa_end;

pa_activity (NumberSelector, pa_ctx(Entry entry; pa_use(EntrySelector)), 
                             Press press, NumberModel& model, Led& led) {
    model.reset();

    do {
        pa_when_abort (is_long_or_double_press(press),
            EntrySelector, press, model.allowedEntryMode(), pa_self.entry, led);

        model.update(pa_self.entry);

    } while (!(model.hasError() || press == Press::LONG));
} pa_end;

pa_activity (GlyphPresenter, pa_ctx(pa_use(Delay)), Led glyph, Led& led) {
    // Hide the leds shortly to introduce next glyph.
    led = led_blank;
    pa_run (Delay, presentation_delay_short);

    // Show the glyph.
    led = glyph;
    pa_run (Delay, presentation_delay_medium);
} pa_end;

pa_activity (ResultPresenter, pa_ctx(pa_use(Delay); pa_use(GlyphPresenter); size_t i; Led glyph), 
                              std::string result, Led& led) {
    while (true) {
        // Present Equal sign.
        led = led_equal;
        pa_run (Delay, presentation_delay_medium);

        // Present result value glyph by glyph.
        for (pa_self.i = 0; pa_self.i < result.size(); ++pa_self.i) {
            {
                const char c = result[pa_self.i];
                switch (c) {
                    case '-': pa_self.glyph = led_minus; break;
                    case '.': pa_self.glyph = led_decimal; break;
                    case ' ': pa_self.glyph = led_blank; break;
                    default: pa_self.glyph = led_digit(c - '0'); break; 
                }
            }
            pa_run (GlyphPresenter, pa_self.glyph, led);
        }

        // Wait a bit before next cycle.
        led = led_blank;
        pa_run (Delay, presentation_delay_long);
    }
} pa_end;

pa_activity (ErrorPresenter, pa_ctx(pa_use(Delay); pa_use(GlyphPresenter); uint8_t i), Led& led) {
    while (true) {
        for (pa_self.i = 0; pa_self.i <= 4; ++pa_self.i) {
            pa_run (GlyphPresenter, led_error(pa_self.i), led);
        }

        // Wait a bit before next cycle.
        led = led_blank;
        pa_run (Delay, presentation_delay_long);
    }
} pa_end;

pa_activity (Calculator, pa_ctx(pa_use(StartAnimator); pa_use(OpSelector); pa_use(NumberSelector);
                                pa_use(ResultPresenter); pa_use(ErrorPresenter);
                                NumberModel first_number, second_number; Op op; Calculator calc), 
                         Press press, Led& led) {
    while (true) {
        // Present animation at start until any button pressed.
        pa_when_abort (is_any_press(press), StartAnimator, led);

        // Get first operand.
        pa_run (NumberSelector, press, pa_self.first_number, led);
        if (pa_self.first_number.hasError()) {
            pa_when_abort (is_any_press(press), ErrorPresenter, led);
            continue;
        }
        
        // Select operator.
        pa_when_abort (is_long_or_double_press(press), OpSelector, press, pa_self.op, led);

        // Get second operand.
        pa_run (NumberSelector, press, pa_self.second_number, led);
        if (pa_self.second_number.hasError()) {
            pa_when_abort (is_any_press(press), ErrorPresenter, led);
            continue;
        }

        // Calculate result (op1 op op2).
        pa_self.calc.calculate(pa_self.first_number, pa_self.op, pa_self.second_number);
        if (pa_self.calc.hasError()) {
            pa_when_abort (is_any_press(press), ErrorPresenter, led);
            continue;
        }

        // Present result until any button pressed.
        pa_when_abort (is_any_press(press), ResultPresenter, pa_self.calc.getFormattedValue(), led);
    };
} pa_end;

pa_activity (LedDriver, pa_ctx(Led prevLed), Led led) {
    while (true) {
        ledPort.write(led);
        pa_self.prevLed = led;
        pa_await (led != pa_self.prevLed);
    }
} pa_end;

pa_activity (Main, pa_ctx(pa_co_res(3); 
                          pa_use(PressRecognizer); pa_use(Calculator); pa_use(LedDriver);
                          Press press; Led led)) {
    pa_co(3) {
        pa_with (PressRecognizer, M5.Btn, pa_self.press);
        pa_with (Calculator, pa_self.press, pa_self.led);
        pa_with (LedDriver, pa_self.led);
    } pa_co_end;
} pa_end;

// Arduino entry points

static pa_use(Main);

void setup() {
    setCpuFrequencyMhz(80);

    M5.begin();
    ledPort.begin(22, 19, 33);
}

void loop() {
    TickType_t prevWakeTime = xTaskGetTickCount();

    while (true) {
        M5.update();

        pa_tick(Main);

        // We run at 10 Hz.
        vTaskDelayUntil(&prevWakeTime, 100);
    }
}
