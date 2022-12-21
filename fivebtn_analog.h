// fivebtn_analog.h
/* Module for identifying input from the five button analog
        card, K845037. */
#ifndef FIVEBTN_H
#define FIVEBTN_H

#include <Arduino.h>

struct button_event {
    byte btn:3;    // One of the 6 button names
    byte status:2; // One of 3 button states
};

class five_btn
{
    private:
        byte last_press = 0;
        unsigned long last_press_time = 0;
    public:
        static const byte NO_BTN = 0;
        static const byte OK_BTN = 1;
        static const byte UP_BTN = 2;
        static const byte DWN_BTN = 3;
        static const byte LFT_BTN = 4;
        static const byte RHT_BTN = 5;

        static const byte ON_BUTTON_HELD = 0;
        static const byte ON_BUTTON_DOWN = 1;
        static const byte ON_BUTTON_UP = 2;

        // Constructor.
        five_btn();

        // Return button event of the buttons
        button_event getButton();

        // Read the analog input and return the const for that input
        // Needs to be debounced for accuracy
        byte read();
};

#endif