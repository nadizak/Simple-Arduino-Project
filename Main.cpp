// main.cpp
/*
    Main script for Project 4 of the bootcamp program.
    Contains the core Arduino setup and lop functions and calls other modules.

    Also contains the main basic Serial communication functions used to interface 
        with the device.

    Jacob Butts
*/
#include <Arduino.h>

#include <DS3231_Simple.h>
#include <EEPROM.h>
#include <Ethernet.h>
#include <FastLED.h>
#include <SimpleDHT.h>
#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal.h>

#include "output.h"
#include "rtc_control.h"
#include "led_control.h"
#include "dht_control.h"
#include "network_control.h"
#include "lcd_ui.h"
#include "token_definitions.h"

#define BAUD_RATE 9600
/*
    EEPROM Usage:
        0 ..... 246 dht_control's temperature logs
        247 ... 272 network_control's saved addresses and such
        273 ... 279 dht_control's saved alarm thresholds
*/

#define DEBUG 0
/*  0..... no debug output
    1..... See simple important steps
    2..... See calls to functions
    3-9... See while loop calls
    Any other numbers are edge cases defined in the code. */
#define VERSION "2.3"

// Keyboard reading constants
#define EOL_BYTE 13
#define BS_BYTE 127
#define ESC_BYTE 27
#define HYPHEN_BYTE 45
#define OPEN_BRACE_BYTE 91
#define UP_ARROW_BYTE 65
struct key_byte {
    bool special;
    byte b:7;
};

// Storage limits
#define NUL '\0'
#define l_SENTENCE 23 // Longest valid command len 18 "add -32765 -32765\0"
#define l_WORD 10 // Longest token len 8 "version\0"
#define l_TOKEN_BUFFER 8 // RGB BYTE 255 BYTE 255 BYTE 255 EOL

#define RGB_POWER_ON_LIGHT 1 

// A row in our lookup table.
// Each piece limited to the size of a byte.
struct LookupEntry {
    char char1, char2;
    uint8_t length:3; // 1-7... 3 bits
    uint8_t token:6; // 0-28... 5 bits
};
// The lookup table itself
LookupEntry lookupTable[19] = {
    {'b','l', 5, t_BLINK},
    {'c','l', 5, t_CLEAR},
    {'d','a', 4, t_DATE},
    {'d','h', 3, t_DHT},
    {'g','r', 5, t_GREEN},
    {'h','e', 4, t_HELP},
    {'i','n', 4, t_INFO},
    {'l','e', 3, t_LED},
    {'l','o', 3, t_LOG},
    {'m','o', 7, t_MONITOR},
    {'o','f', 3, t_OFF},
    {'o','n', 2, t_ON},
    {'r','e', 3, t_RED},
    {'r','g', 3, t_RGB},
    {'t','e', 5, t_SCALE},
    {'s','e', 3, t_SET},
    {'t','i', 4, t_TIME},
    {'v','e', 7, t_VERSION},
    {'y','e', 6, t_YELLOW}
};

led_control LED;
rtc_control RTC;
dht_control DHT;
network_control NET;
lcd_ui LCD_UI;

Output out;

// For direct user input saving
char input_buffer[l_SENTENCE];
uint8_t input_length = 0;
char last_input[l_SENTENCE];
uint8_t last_length = 0;
bool press_any_key = false;

// For user input translation
uint8_t token_buffer[l_TOKEN_BUFFER];
uint8_t token_length = 0;

// Used for certain functions to halt processing
bool error_flag = false;

word atot(char*, uint8_t);
bool capturableByte(byte);
void commandError();
void parseInput(char*, uint8_t);
void parseTokens();
bool processInput();
void resetInputBuffer();
key_byte s_readChar();

// ====== //
// Intitial setup function
void setup() {
    Serial.begin(BAUD_RATE);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }
    #if DEBUG >= 1
    Serial.println(F("Program start."));
    #endif
    NET.setup();
    LED.setup();
    RTC.setup();
    DHT.setup(&RTC);
    resetInputBuffer();

    // Turn on the power light
    LED.setRGBColor(RGB_POWER_ON_LIGHT, 0, 50, 0);

    LCD_UI.setup();
}

// Looping routine with core functions
void loop() {
    LED.loop();
    DHT.loop();

    // Process Command Line Input
    if (processInput()) {
        parseInput(input_buffer, input_length);
        #if DEBUG >= 1
        Serial.print("CLI Token buffer: ");
        for (int i=0; i<l_TOKEN_BUFFER; i++) {
            Serial.print(token_buffer[i]);
            Serial.print(" ");
        }
        Serial.print("\n\rToken buffer length: ");
        Serial.println(token_length);
        #endif
        parseTokens();
        resetInputBuffer();
    }

    // Process incoming network input
    if (NET.loop()) {
        out.udpBegin();
        parseInput(NET.getPacketBuffer(), NET.getPacketBufferLength());
        #if DEBUG >= 1
        Serial.print("Network Token buffer: ");
        for (int i=0; i<l_TOKEN_BUFFER; i++) {
            Serial.print(token_buffer[i]);
            Serial.print(" ");
        }
        Serial.print("\n\rToken buffer length: ");
        Serial.println(token_length);
        Serial.print("UDP packet bytes remaining: ");
        Serial.print(NET.getUDP()->parsePacket());
        #endif
        parseTokens();
        out.udpEnd();
    }

    LCD_UI.loop();
}

// ============================== //
// Returns true if it's a character we wish to save to input buffer
bool capturableByte(byte b) {
    return (isAlphaNumeric(b) ||
            isSpace(b) ||
            b == HYPHEN_BYTE);
}

// Prints an error line fr when CLI 
void commandError() {
    out.println(F("Incorrect or unknown commands/options. Run help for details."));
}

// Convert a string of numbers to 16 or 8 bit number for token storage
// Should return smallest option with top byte being 0
// Set errorflag to true if the string was invalid
word atot(char *input, uint8_t len) {
    error_flag = false;
    #if DEBUG >= 2
    Serial.print(F("atot start, input: "));
    Serial.println(input);
    #endif
    int32_t number = 0;
    uint8_t i = 0, charbyte = 0;

    if (len > 5) {
        out.println(F("Max length of number exceeded."));
        out.println(len);
        error_flag = true;
    }
    while (i <= len && input[i] != NUL) {
        // Convert char to byte number
        charbyte = byte(input[i]) - 48;
        #if DEBUG >= 3
        Serial.print(F("atot while loop "));
        Serial.println(charbyte);
        #endif
        if (charbyte >= 0 && charbyte < 10) {
            number = (number * 10) + charbyte;
            #if DEBUG >= 5
            Serial.println(number);
            #endif
        }
        else {
            out.println(F("Non-number entered."));
            error_flag = true;
            return number;
        }
        i++;
    }
    if (number > 65535) {
        out.println(F("Number exceeds limit."));
        error_flag = true;
    }
    #if DEBUG >= 2
    Serial.print(F("atot end, result (unsigned): "));
    Serial.println(number);
    #endif
    return number;
}

// Read the input buffer and create the token buffer
void parseInput(char* input, uint8_t length) {
    #if DEBUG >= 2
    Serial.println(F("parseInput start, input and length:"));
    Serial.println(input);
    Serial.println(input_length);
    #endif
    char word[l_WORD];
    token_length = 0;
    uint8_t i = 0, c = 0;
    bool save_int = false;
    // Read chars until we reach the end of the input
    while (i <= length) {
        #if DEBUG >= 4
        Serial.print(F("parseInput while loop char: "));
        Serial.println(input[i]);
        #endif
        // If it is a non-space char we save it off
        if (input[i] != ' ' && input[i] != NUL) {
            word[c++] = input[i];
        }
        else {
            // We hit a space or eol and can end our current word
            word[c] = NUL;
            #if DEBUG >= 3
            Serial.print(F("parseInput while loop word: "));
            Serial.println(word);
            #endif
            if (c >= 2) {
                // Check list of tokens for a match
                for (LookupEntry e : lookupTable) {
                    if (word[0] == e.char1 &&
                        word[1] == e.char2 &&
                        strlen(word) == e.length) {
                            token_buffer[token_length++] = e.token;
                            #if DEBUG >= 8990
                            Serial.print(F("Token found: "));
                            Serial.println(e.token);
                            #endif
                            // Reset by zeroing out relevant data
                            c = 0;
                            word[0] = word[1] = NUL;
                            // Special case token
                            // Any of these tokens, when encountered, will
                            // Attempt to convert unknown text in the input buffer
                            // into a word or byte to store in the token buffer
                            switch (e.token) {
                                case t_WORD:
                                case t_BYTE:
                                case t_SET:
                                case t_RGB:
                                case t_LED:
                                    #if DEBUG >= 3
                                    Serial.println(F("save int flag set"));
                                    #endif
                                    save_int = true;
                            }
                            break;
                    }
                }
            }
            // If word still exists, no token was found...
            // and if save_int is true, store as byte/word if it converts to int
            if (c > 0 && save_int) {
                #if DEBUG >= 3
                Serial.print(F("Non-token taken in for set/word token: "));
                Serial.println(word);
                #endif
                switch (token_buffer[0]) {
                    uint16_t word_num;
                    default:
                        word_num = atot(word, c);
                        if ( error_flag );
                        else if (highByte(word_num) == 0) {
                            token_buffer[token_length++] = t_BYTE;
                            token_buffer[token_length++] = lowByte(word_num);
                        }
                        else {
                            token_buffer[token_length++] = t_WORD;
                            token_buffer[token_length++] = highByte(word_num);
                            token_buffer[token_length++] = lowByte(word_num);
                        }
                        break;
                }
            }
            // Reset the word
            c = 0;
            word[0] = word[1] = NUL;
        }
        i++;
    }
    #if DEBUG >= 2
    Serial.println(F("parseInput end"));
    #endif
    token_buffer[token_length] = t_EOL;
}

// Read the token buffer and perform the appropriate action
void parseTokens() {
    if (token_length == 0) {
        commandError();
        return;
    }
    // Branch to options
    switch (token_buffer[0]) {
        case t_LED:
        case t_RGB:
            switch (token_buffer[1]) {
                case t_EOL:
                    LED.printStatus();
                    break;
                case t_ON:
                case t_OFF:
                case t_BLINK:
                    LED.setLightStatus(token_buffer[1]);
                    break;
                // Static colors
                case t_RED:
                case t_GREEN:
                case t_YELLOW:
                    LED.setRGBColor(
                        token_buffer[1] == t_RED || token_buffer[1] == t_YELLOW? 255: 0,
                        token_buffer[1] == t_GREEN || token_buffer[1] == t_YELLOW? 255: 0,
                        0);
                    break;
                case t_BYTE:
                    if (token_buffer[1] != t_BYTE || token_buffer[3] != t_BYTE ||
                            token_buffer[5] != t_BYTE)
                        commandError();
                    LED.setRGBColor(token_buffer[2], token_buffer[4], token_buffer[6]);
                    break;
                default:
                    commandError();
            }
            break;
        /* ======= */
        case t_DHT:
            switch (token_buffer[1]) {
                case t_EOL:
                    DHT.printStatus(out);
                    break;
                case t_MONITOR:
                    if (!out.udp_print) {
                        DHT.toggleMonitor();
                        press_any_key = true;
                    }
                    break;
                case t_LOG:
                    switch (token_buffer[2]) {
                        case t_EOL:
                            DHT.printLogs();
                            break;
                        case t_INFO:
                            DHT.printLogInfo();
                            break;
                        case t_CLEAR:
                            DHT.clearLog();
                            break;
                    }
                    break;
            }
            break;
        /* ======= */
        case t_TIME:
        case t_DATE:
            RTC.printStatus();
            break;
        /* ======= */
        case t_SET:
            switch (token_buffer[1]) {
                case t_TIME:
                case t_DATE:
                    if (token_buffer[2] == t_BYTE &&
                            token_buffer[4] == t_BYTE &&
                            token_buffer[6] == t_BYTE) {
                        if (token_buffer[1] == t_TIME)
                            RTC.setTime(token_buffer[3], token_buffer[5], token_buffer[7]);
                        if (token_buffer[1] == t_DATE)
                            RTC.setDate(token_buffer[3], token_buffer[5], token_buffer[7]);
                    }
                    else
                        commandError();
                    break;
                case t_BLINK:
                    // Require number token to proceed
                    if (token_buffer[2] == t_WORD)
                        LED.setBlinkRate(word(token_buffer[3], token_buffer[4]));
                    else if (token_buffer[2] == t_BYTE)
                        LED.setBlinkRate(token_buffer[3]);
                    else
                        commandError();
                    break;
                case t_DHT:
                    if (token_buffer[2] == t_SCALE && token_buffer[3] == t_BYTE)
                        DHT.setToFahrenheit(token_buffer[3] == 1);
                    else
                        commandError();
                    break;
            }
            break;
        /* ======= */
        case t_VERSION:
            out.println(VERSION);
            break;
        /* ======= */
        case t_HELP:
            out.println(F(
                "Commands (square brackets denote options):\n\r"
                "\tLED, DHT, TIME, DATE, VERSION, HELP"
                "\tLED [on|off|red|green|yellow|blink]\n\r"
                "\tRGB <0-255> <0-255> <0-255> (RGB values)\n\r"
                "\tDHT [MONITOR|LOG]\n\r"
                "\tDHT LOG\n\r\tDHT LOG [INFO|CLEAR]\n\r"
                "\tSET TIME <HH> <MM> <SS>\n\r"
                "\tSET DATE <YY> <MM> <DD>\n\r"
                "\tSET DHT SCALE [0|1] (Celcius or Fahrenheit)\n\r"
                "\tSET BLINK <number 0-65535>\n\r"
            ));
            break;
        default:
            commandError();
    }
    // Save off input if we wish to recall it
    for (uint8_t i=0; i<input_length; i++) {
        last_input[i] = input_buffer[i];
    }
    last_length = input_length;
}

// Check for and read user input and save it to the input buffer
// Returns true if the input buffer is complete
bool processInput() {
    key_byte input;
    // First check for user input
    input = s_readChar();
    if (!input.b) return false;
    else if (press_any_key) {
        if (DHT.monitor) {
            DHT.toggleMonitor();
        }
        press_any_key = false;
        resetInputBuffer();
        return false;
    }
    // Backspace
    else if (input.b == BS_BYTE) {
        if (input_length > 0) {
            Serial.print(F("\b \b"));
            input_length--;
        }
        return false;
    } // Enter key
    else if (input.b == EOL_BYTE) {
        Serial.println();
        input_buffer[input_length] = NUL;
        if (input_length == 0) {
            resetInputBuffer();
            return false; // Do nothing if nothing entered
        }
    } // Up arrow
    else if (input.special && input.b == UP_ARROW_BYTE) {
        // Erase previous text
        for (uint8_t i=0; i<input_length; i++) {
            Serial.print(F("\b \b"));
        }
        // Copy andd print from last input
        for (uint8_t i=0; i<last_length; i++) {
            Serial.print(last_input[i]);
            input_buffer[i] = last_input[i];
        }
        input_length = last_length;
        return false;
    } // Capture regular output bytes
    else if(input_length < l_SENTENCE && capturableByte(input.b)) {
        char byteChar = char(input.b);
        Serial.print(byteChar);
        input_buffer[input_length++] = tolower(byteChar);
        // Return to stop processing the input further
        return false;
    }
    else {
        // Input is at capacity, ignore new input
        return false;
    }
    return true;
}

// Clears input buffer
void resetInputBuffer() {
    #if DEBUG >= 2
    Serial.print(F("resetInputBuffer"));
    #endif
    input_length = 0;
    input_buffer[0] = NUL;
    if (press_any_key)
        Serial.println(F("Press any key to continue...\n"));
    else
        Serial.print(F("\n\r>"));
}

// Read a single byte in via Serial. If special keys are read we may
//      set flags to handle it on future calls
key_byte s_readChar() {
    static bool escaped = false;
    byte incomingByte = 0;
    if (Serial.available() > 0) {
        incomingByte = Serial.read();
        #if DEBUG >= 9000
        // Print byte of keyboard input
        Serial.print(incomingByte);
        #endif
        // Check for special keys we want to handle
        // Escape byte and [ character come arrow key
        if (incomingByte == ESC_BYTE ||
            (escaped && incomingByte == OPEN_BRACE_BYTE)) {
            escaped = true;
            // Ignore
            return {false, 0};
        }
        // Check for Up arrow special key
        else if (escaped) {
            escaped = false;
            if (incomingByte == UP_ARROW_BYTE)
                return {true, incomingByte};
            else
                return {false, 0};
        }
    }
    return {false, incomingByte};
}
