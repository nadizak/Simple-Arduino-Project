// led_control.cpp
#include "led_control.h"

#define DEBUG 0

#define RGB_DEFAULT_LIGHT 0

// Out is used for any outward output in response to a function call
// and will ouput to serial or udp depending on Output's setting
extern Output out;

void led_control::setup() {
    blink_rate = 500;
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);

    for (uint8_t i=0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Black;
        leds_mem[i] = CRGB::Blue;
    }
    FastLED.show();
}

void led_control::loop() {
    bool check_for_blink = false;
    for (uint8_t i=0; i < NUM_LEDS; i++) {
        if (led_states[i] == t_BLINK) {
            check_for_blink = true;
            break;
        }
    }
    if (check_for_blink) {
        // Divide millis into "number of blink_rates that have passed"
        // Then set light based on odd or even
        // If flag is on and we are on an even iterval
        //      or if flag is off and we are on an odd we run toggle
        if ((blink_flag && ((millis() / blink_rate) % 2 == 0)) ||
            (!blink_flag && ((millis() / blink_rate) % 2 == 1)) ) {
            blink_flag = !blink_flag;
            for (uint8_t i=0; i < NUM_LEDS; i++) {
                if (led_states[i] == t_BLINK)
                    toggleLight(i);
            }
        }
    }
}

void led_control::printStatus() {
    for (uint8_t i=0; i < NUM_LEDS; i++) {
        out.print(F("Light "));
        out.print(i);
        out.print(" set to ");
        switch (led_states[i]) {
            case t_ON:
                out.print(F("on"));
                break;
            case t_OFF:
                out.print(F("off"));
                break;
            case t_BLINK:
                out.print(F("blink"));
        }
        out.print(F(", color is rgb("));
        out.print(leds[i].red);
        out.print(F(", "));
        out.print(leds[i].green);
        out.print(F(", "));
        out.print(leds[i].blue);
        out.println(F(")"));
    }
    out.print(F("Blink Rate: "));
    out.println(blink_rate);
}

void led_control::setBlinkRate(word w) {
    #if DEBUG >= 1
    Serial.print(F("LED.setBlinkRate "));
    Serial.println(w);
    #endif
    blink_rate = w;
}

void led_control::setLightStatus(byte b) {
    setLightStatus(RGB_DEFAULT_LIGHT, b);
}

void led_control::setLightStatus(uint8_t light, byte b) {
    #if DEBUG >= 1
    Serial.print(F("LED.setLightStatus "));
    Serial.print(light); Serial.print(F(" "));
    Serial.println(b);
    #endif
    if (light >= NUM_LEDS) {
        out.println(F("Invalid light value"));
        return;
    }
    led_states[light] = b;
    if (b == t_ON || b == t_OFF) {
        // Set color to either prev color or black/off
        leds[light] = (led_states[light] == t_ON)? leds_mem[light]: CRGB::Black;
        FastLED.show();
    }
    // Blink state will be handled in loop calls and only needs the state set
}

void led_control::setRGBColor(byte r, byte g, byte b) {
    setRGBColor(RGB_DEFAULT_LIGHT, r, g, b);
}

void led_control::setRGBColor(uint8_t light, byte r, byte g, byte b) {
    leds[light] = CRGB(r, g, b);
    leds_mem[light] = leds[light];
    if (led_states[light] != t_BLINK)
        led_states[light] = (r+g+b > 1)? t_ON: t_OFF;
    FastLED.show();
}

void led_control::toggleLight(uint8_t light) {
    #if DEBUG >= 2
    Serial.print(F("LED.toggle "));
    Serial.println(light);
    #endif
    if (light >= NUM_LEDS) {
        out.println(F("Invalid light value"));
        return;
    }
    switch (led_states[light]) {
        case t_ON:
            leds[light] = CRGB::Black;
            led_states[light] = t_OFF;
            break;
        case t_OFF:
            leds[light] = leds_mem[light];
            led_states[light] = t_ON;
            break;
        case t_BLINK:
            bool is_on = leds[light] == leds_mem[light];
            leds[light] = is_on? CRGB::Black: leds_mem[light];
    }
    FastLED.show();
}