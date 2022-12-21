// lcd_ui.h
/* Handles the LCD display as well as executing actions from
    the this UI using the analog buttons. */
#ifndef LCDUI_H
#define LCDUI_H

#include <Arduino.h>
#include <LiquidCrystal.h>
#include "token_definitions.h"
#include "fivebtn_analog.h"
#include "rtc_control.h"
#include "dht_control.h"

#define MENU_EOL 255

#define SCR_DHT 0
#define SCR_LOGS 1
#define SCR_NETSTAT 2
#define SCR_CONFIG 3

#define CFG_TEMPS 0
#define CFG_IP 1
#define CFG_SUBNET 2
#define CFG_GATEWAY 3
#define CFG_CLEARLOGS 4

#define RS_PIN 7
#define EN_PIN 6
#define D4_PIN 5
#define D5_PIN 4
#define D6_PIN 3
#define D7_PIN 2

class lcd_ui
{
    private:
        LiquidCrystal lcd = LiquidCrystal(RS_PIN, EN_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN);
        five_btn analog;
        uint8_t menu_state[3] = {0, MENU_EOL, MENU_EOL};
        uint8_t editable_ints[4];
        uint8_t menu_depth = 0;
        uint8_t confirm_ct = 100;
        bool editable_ints_active = false;
    public:
        // Arduino Setup Call
        void setup();

        // Arduino Loop Call
        // Return true if token buffer needs to be processed
        bool loop();

        // Process analog input for editable ints state
        /* The editable ints state stores 4 byte sized ints into the
                editable ints variable. The retrive/edit/saving is handled in this
                function.
            Temperature int's are retrieved and saved with a +-40 offset to contain it
                within the byte (DHT22 min of -40 max of 177 F) and also printed
                with this offset in mind. e.g. (0 is -40, 217 is 177)*/
        bool editableIntsInputProcess();

        // Return max menu index depending on flags
        // Scrolling of the menu state does have a max of MENU_EOL (255)
        uint8_t getCurrentMaxState();

        // Return true if menu state is home screen
        bool isHomeScreen();

        // checks if menu state is currently set to
        //  scrollable of the given menu
        bool onSubMenu(byte);

        // Update screen using current values
        void updateScreen();

        // Go back to home screen
        void returnToHomeScreen();

        // Process analog input for standard menu situation
        bool standardMenuInputProcess();

        // Write given object to the LCD screen at current position
        void writeTempHum_to_LCD(float, uint8_t);
        void writeTime_to_LCD(DateTime);

        // Write common things to the LCd screen in specific spots
        void writeNextArrow();
        void writeDownToEnter();
};

#endif