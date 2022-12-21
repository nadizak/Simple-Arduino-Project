// fivebtn_analog.cpp
#include "fivebtn_analog.h"

#define DEBUG 0

#define DEBOUNCE_DELAY 100

#define CONFIRM_VOLT_L 730
#define CONFIRM_VOLT_U 750
#define RIGHT_VOLT_L 490
#define RIGHT_VOLT_U 510
#define DOWN_VOLT_L 320
#define DOWN_VOLT_U 340
#define UP_VOLT_L 140
#define UP_VOLT_U 160
#define LEFT_VOLT_L 0
#define LEFT_VOLT_U 20

five_btn::five_btn() {
    pinMode(A0, INPUT);
}

button_event five_btn::getButton() {
    // Get button pressed
    byte button_press = read();
    button_event result = {0 , 0};

    if (millis() - last_press_time > DEBOUNCE_DELAY) {
        #if DEBUG == 4
        if (button_press) Serial.println(button_press);
        #endif
        // Button state has persisted for longer than our delay
        // Check what state it is and return value accordingly
        // Button down
        if (button_press && last_press == NO_BTN) {
            result.btn = button_press;
            result.status = ON_BUTTON_DOWN;
            #if DEBUG >= 1
            Serial.print("Button Down: ");
            Serial.println(result.btn);
            #endif
        }
        // Button up
        else if (last_press && button_press == NO_BTN) {
            result.btn = last_press;
            result.status = ON_BUTTON_UP;
            #if DEBUG >= 1
            Serial.print("Button Up: ");
            Serial.println(result.btn);
            #endif
        }
        // Otherwise button held
        else {
            result.btn = button_press;
            result.status = ON_BUTTON_HELD;
            #if DEBUG >= 1
            if (button_press != NO_BTN) {
                Serial.print("Button Held: ");
                Serial.println(result.btn);
            }
            #endif
        }
    }

    // If different from last, set timer
    if (button_press != last_press)
        last_press_time = millis();

    // Save off reading to check in future call
    last_press = button_press;

    return result;
}

byte five_btn::read() {
    int reading = analogRead(A0);
    #if DEBUG == 3
        Serial.println(reading);
    #endif
    if (reading > CONFIRM_VOLT_L &&
            reading < CONFIRM_VOLT_U)
        return OK_BTN;
    if (reading > RIGHT_VOLT_L &&
            reading < RIGHT_VOLT_U)
        return RHT_BTN;
    if (reading > DOWN_VOLT_L &&
            reading < DOWN_VOLT_U)
        return DWN_BTN;
    if (reading > UP_VOLT_L &&
            reading < UP_VOLT_U)
        return UP_BTN;
    if (reading > LEFT_VOLT_L &&
            reading < LEFT_VOLT_U)
        return LFT_BTN;
    #if DEBUG == 2
    if (reading >= 10)
        Serial.println(reading);
    #endif
    return NO_BTN;
}