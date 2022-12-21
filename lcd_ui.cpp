// lcd_ui.cpp
#include "lcd_ui.h"

#define DEBUG 0

#define SCREEN_UPDATE_DELAY 5000

// Clock and Temp sensor are pulled from parent script.
extern rtc_control RTC;
extern dht_control DHT;
extern network_control NET;

void lcd_ui::setup() {
    lcd.begin(16,2);
    returnToHomeScreen();

    static const byte upDownArrow[8] = {
        0b00100,
        0b01010,
        0b10001,
        0b00000,
        0b00000,
        0b10001,
        0b01010,
        0b00100,
    };
    lcd.createChar(2, upDownArrow);
}

bool lcd_ui::loop() {
    // Run input processing and update scren if needed
    if ((editable_ints_active && editableIntsInputProcess()) ||
            (!editable_ints_active && standardMenuInputProcess()))
        updateScreen();

    static unsigned long loop_delay = 0;
    // We only periodically redraw screen for these conditions
    if (isHomeScreen() && millis() - loop_delay > SCREEN_UPDATE_DELAY) {
        loop_delay = millis();
        updateScreen();
    }
    return false;
}

bool lcd_ui::editableIntsInputProcess() {
    // Must call button down check first
    button_event event = analog.getButton();

    if (event.btn == five_btn::NO_BTN)
        return false;

    // Up and down change currently held value, uses btn press
    if (event.btn == five_btn::UP_BTN || event.btn == five_btn::DWN_BTN) {
        editable_ints[menu_state[2]] += (event.btn == five_btn::DWN_BTN)? -1: 1;
        // No need to check for out of bounds as the
        // byte itself will handle it with overflow/underflow
        return true;
    }

    // On btn down we shift number held
    else if (event.status == five_btn::ON_BUTTON_DOWN &&
            (event.btn == five_btn::LFT_BTN || event.btn == five_btn::RHT_BTN)) {
        menu_state[2] += (event.btn == five_btn::LFT_BTN)? -1: 1;;
        // If we go too far, to the left or right, return to settings
        if ((menu_state[2] > 3) || (menu_state[2] + 1 == 0)) {
            editable_ints_active = false;
            menu_state[2] = MENU_EOL;
        }
        return true;
    }

    // Last case is if we pressed the save button
    else if (event.btn == five_btn::OK_BTN && event.status == five_btn::ON_BUTTON_UP) {
        IPAddress newAddress;
        switch (menu_state[1]) {
            case CFG_GATEWAY:
            case CFG_SUBNET:
            case CFG_IP:
                for (int i = 0; i < 4; i++)
                    newAddress[i] = editable_ints[i];
        }
        switch (menu_state[1]) {
            case CFG_GATEWAY:
                NET.saveGatewayAddr(newAddress);
                break;
            case CFG_SUBNET:
                NET.saveSubnetAddr(newAddress);
                break;
            case CFG_IP:
                NET.saveLocalIPAddr(newAddress);
                break;
            case CFG_TEMPS:
                DHT.setAlarmGates(editable_ints[0] - 40,
                                    editable_ints[1] - 40,
                                    editable_ints[2] - 40,
                                    editable_ints[3] - 40);
                break;
        }
        // Return to settings
        editable_ints_active = false;
        menu_state[2] = MENU_EOL;
        return true;
    }
    return false;
}

uint8_t lcd_ui::getCurrentMaxState() {
    if (onSubMenu(SCR_LOGS))
        return max(0, DHT.getEntriesCount() - 1);
    else if (onSubMenu(SCR_CONFIG))
        return CFG_CLEARLOGS;
    else
        return SCR_CONFIG;
}

bool lcd_ui::isHomeScreen() {
    return (menu_state[0] == 0 &&
            menu_state[1] == MENU_EOL &&
            menu_state[2] == MENU_EOL);
}

bool lcd_ui::onSubMenu(byte menu) {
    switch (menu) {
        case SCR_CONFIG:
        case SCR_LOGS:
            return (menu_state[0] == menu &&
                    menu_state[1] != MENU_EOL &&
                    menu_state[2] == MENU_EOL);
        default:
            return false;
    }
}

void lcd_ui::returnToHomeScreen() {
    menu_state[0] = 0;
    menu_state[1] = menu_state[2] = MENU_EOL;
    menu_depth = 0;
    editable_ints_active = false;
    updateScreen();
}

bool lcd_ui::standardMenuInputProcess() {
    button_event event = analog.getButton();
    if (event.btn == five_btn::NO_BTN || event.status != five_btn::ON_BUTTON_UP)
        return false;
    // Normal menu navigation mode
    switch (event.btn) {
        // Go back
        case five_btn::UP_BTN:
        returnToHomeScreen();
        break;

        // Cycles choices
        case five_btn::LFT_BTN:
            // Go back to previous (or last screen if on first)
            if (menu_state[menu_depth] == 0)
                menu_state[menu_depth] = getCurrentMaxState();
            else
                menu_state[menu_depth]--;
            break;
        case five_btn::RHT_BTN:
            menu_state[menu_depth]++;
            // Have we gone past the last screen?
            if (menu_state[menu_depth] > getCurrentMaxState())
                menu_state[menu_depth] = 0;
            break;

        // Okay button is used to confirm and enter submenus
        case five_btn::OK_BTN:
            // Confirm clear logs check
            if (onSubMenu(SCR_CONFIG) && menu_state[1] == CFG_CLEARLOGS) {
                if (confirm_ct >= 10)
                    confirm_ct = 10;
                confirm_ct--;
                if (confirm_ct == 0)
                    DHT.clearLog();
            }
            // Enter logs and config submenus check
            else if (menu_depth == 0) {
                if (menu_state[0] == SCR_LOGS) {
                    menu_depth++;
                    menu_state[1] = 0; // Needs to not be EOL for get max
                    menu_state[1] = getCurrentMaxState();
                }
                else if (menu_state[0] == SCR_CONFIG) {
                    menu_depth++;
                    menu_state[1] = 0;
                }
            }
            // All config screens but clear logs enter into edit ints mode
            else if (menu_depth == 1 &&
                        menu_state[0] == SCR_CONFIG &&
                        menu_state[1] != CFG_CLEARLOGS) {
                // Set special state flag and retrieve the data
                editable_ints_active = true;
                menu_state[2] = 0;
                for (uint8_t i = 0; i < 4; i++) {
                    switch (menu_state[1]) {
                        case CFG_GATEWAY:
                            editable_ints[i] = NET.gateway_addr[i];
                            break;
                        case CFG_IP:
                            editable_ints[i] = NET.local_ip[i];
                            break;
                        case CFG_TEMPS:
                            // Add 40 to the 16bit int we get and cast it to 8 bit
                            // We do this because we can only show up to 255 for the
                            // editable int screen but that is within out sensor range
                            // anyway
                            editable_ints[i] = (int8_t)(DHT.alarm_gates[i] + 40);
                            break;
                        case CFG_SUBNET:
                            editable_ints[i] = NET.subnet_addr[i];
                            break;
                    }
                }
            }
            break;
    }
    return true;
    // We increment this as a psuedo reset for the confirmation dialog
    confirm_ct++;
}

void lcd_ui::updateScreen() {
    #if DEBUG >= 2
    if ( !(isHomeScreen())) {
        Serial.print(F("Menu Change: "));
        for(int i=0; i<3; i++) {
            Serial.print(menu_state[i]);
            Serial.print(' ');
        }
        Serial.print(F("Depth "));
        Serial.println(menu_depth);
    }
    #endif
    lcd.clear();
    lcd.setCursor(0, 0);
    // Editable ints state
    if (editable_ints_active) {
        // menu_state[2] 0-3 determines which int we edit
        uint8_t current_int = menu_state[2];
        for (int i = 0; i < 4; i++) {
            if (i == current_int) {
                lcd.setCursor(i*4, 0);
                lcd.write(2); // up/down editable arrow
                // Print value (-40 if temperature)
                lcd.print((int)editable_ints[i] - (menu_state[1] == CFG_TEMPS? 40: 0));
                lcd.setCursor(i*4 + 3, 1);
                lcd.print(F(">"));
                continue;
            }
            lcd.setCursor(i*4, 1);
            // Print value (-40 if temperature)
            lcd.print((int)editable_ints[i] - (menu_state[1] == CFG_TEMPS? 40: 0));
            if (i < 3) {
                if (i + 1 == current_int)
                    lcd.print(F("<"));
                else
                    lcd.print(F("."));
            }
        }

    }
    // Check for sub menus
    else if (onSubMenu(SCR_LOGS)) {
        if (getCurrentMaxState() == 0) {
            lcd.print(F("No logs."));
        }
        else {
            lcd.write(0x7F); // Left arrow
            lcd.setCursor(5, 0);
            log_entry entry = DHT.getLogEntry(menu_state[1]);
            writeTime_to_LCD(entry.ts);
            writeNextArrow();
            lcd.setCursor(0, 1);
            lcd.print(menu_state[1] + 1);
            lcd.print(F(":"));
            lcd.setCursor(4, 1);
            writeTempHum_to_LCD(entry.temp, entry.humid);
        }
    }
    else if (onSubMenu(SCR_CONFIG)) {
        switch (menu_state[1]) {
            case CFG_CLEARLOGS:
                lcd.print(F("Clear Logs?"));
                if (confirm_ct < 10) {
                    lcd.setCursor(11, 0);
                    lcd.print("...");
                    lcd.print(confirm_ct);
                }
                lcd.setCursor(0, 1);
                if (confirm_ct <= 0)
                    lcd.print(F("  Cleared!"));
                else
                    lcd.print(F("Push X to clear"));
                break;
            case CFG_IP:
                lcd.print(F("Change IP Addr"));
                writeDownToEnter();
                break;
            case CFG_TEMPS:
                lcd.print(F("Temp. Alarm"));
                lcd.setCursor(0,1);
                lcd.print(F("Gates"));
                writeDownToEnter();
                break;
            case CFG_SUBNET:
                lcd.print(F("Config Subnet"));
                writeDownToEnter();
                break;
            case CFG_GATEWAY:
                lcd.print(F("Config Gateway"));
                writeDownToEnter();
                break;
        }
        writeNextArrow();
    }
    // Top level menus
    else {
        switch (menu_state[0]) {
            case SCR_DHT:
                writeTempHum_to_LCD(DHT.temperature, DHT.humidity);
                lcd.setCursor(11, 1); // 11 = 16 - len("12:59")
                writeTime_to_LCD(RTC.readTime());
                break;
            case SCR_LOGS:
                lcd.print(F("DHT Logs"));
                writeDownToEnter();
                break;
            case SCR_NETSTAT:
                lcd.print(F("Ntwrk Packets"));
                lcd.setCursor(0, 1);
                lcd.print(F("Snt:"));
                lcd.print(NET.packetsSent);
                lcd.print(F(" Rcv:"));
                lcd.print(NET.packetsRcvd);
                break;
            case SCR_CONFIG:
                lcd.print(F("Config"));
                writeDownToEnter();
                break;
        }
        writeNextArrow();
    }
}

void lcd_ui::writeTempHum_to_LCD(float temp, uint8_t humid) {
    lcd.print(DHT.toFahrenheit(temp));
    lcd.write(0xDF); // Degree symbol
    lcd.print(F("F, "));
    lcd.print((int)humid);
    lcd.print(F("%RH"));
}

void lcd_ui::writeTime_to_LCD(DateTime ts) {
    lcd.print(ts.Hour);
    lcd.print(F(":"));
    if (ts.Minute < 10)
        lcd.print(F("0"));
    lcd.print(ts.Minute);
}

void lcd_ui::writeNextArrow() {
    lcd.setCursor(15, 0);
    lcd.write(0x7E); // Right arrow
}

void lcd_ui::writeDownToEnter() {
    lcd.setCursor(6, 1);
    lcd.print(F("X to enter"));
}