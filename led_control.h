// led_control.h
/* Class to control the LEDs on the Arduino board. */
#ifndef LED_H
#define LED_H

#include <Arduino.h>
#include <FastLED.h>
#include "output.h"
#include "token_definitions.h"

//FastLED
#define NUM_LEDS 4
#define DATA_PIN 9

class led_control
{
    private:
        CRGB leds[NUM_LEDS];
        CRGB leds_mem[NUM_LEDS];
        byte led_states[NUM_LEDS];
        word blink_rate = 500;
        bool blink_flag = false;
    public:
        // Arduino setup calls
        void setup();

        // Called in arduino's loop. Mostly handles blinks
        void loop();

        // Print current status of all lights
        void printStatus();

        // Adjust blink rate of all lights
        void setBlinkRate(word);

        // Sets a light's status. Options are on, off, blink.
        void setLightStatus(byte, byte);
        // Sets the default adjustable light
        void setLightStatus(byte);

        // Sets RGB light to a specific color value
        void setRGBColor(byte, byte, byte);
        void setRGBColor(uint8_t, byte, byte, byte);

        // A mostly internal function that simply flips the light
        //      for the blinking status
        void toggleLight(byte);
};

#endif