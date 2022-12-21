// dht_control.cpp
#include "dht_control.h"
#include "led_control.h"
#include "output.h"

#define DEBUG 0
/*  0..... no debug output
    1..... See simple important steps
    2..... See periodic messages that may interupt CLI display */

#define DHT_PIN 8
#define READ_DELAY 5 // in seconds
#define LOG_DELAY 15 // in minutes
#define MAX_LOG_BYTES 247 // Limited by our byte sized index
#define MAX_LOG_ENTRIES 27 // (247 - 4) / (9, size of entry)
#define RGB_ALARM_LIGHT 2
// EEPROM logs use byte 0 through 246
#define EEPROM_LOGS 0
// Saved alarm data fills bytes 273 through 279
#define EEPROM_ALARMS 273

// Out is used for any outward output in response to a function call
// and will ouput to serial or udp depending on Output's setting
extern Output out;
// Pull the LED control from main.cpp to toggle our alarm light
extern led_control LED;

void dht_control::setup(rtc_control *ptr) {
    rtc_ptr = ptr;
    dht22 = SimpleDHT22(DHT_PIN);
    EEPROM.get(EEPROM_LOGS, log_index);
    EEPROM.get(EEPROM_LOGS + sizeof(int), log_entries);
    alarm_state = 0;
    LED.setRGBColor(RGB_ALARM_LIGHT, 0, 50, 0);
    #if DEBUG > 1
    //clearLog();
    #endif
    // Call loop immedietly to get a reading
    // Read EEPROM for alarm gates
    for (int i = 0; i < 4; i ++) {
        EEPROM.get(EEPROM_ALARMS + i * sizeof(int), alarm_gates[i]);
        #if DEBUG >= 1
        Serial.print("Alarm Gate ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(alarm_gates[i]);
        #endif
    }
    loop();
}

void dht_control::loop() {
    // Read delay stores the modulo result on millis
    // New read is done if this number loops back to 0
    static unsigned int read_delay = 1000*READ_DELAY;
    unsigned int read_check = millis() % (1000*READ_DELAY);
    if (read_check > read_delay) return;
    read_delay = read_check;

    // Read in data
    int err = SimpleDHTErrSuccess;
    if ((err = dht22.read2(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
        if (monitor) {
            Serial.print(F("DHT read failed, err=")); Serial.print(SimpleDHTErrCode(err));
            Serial.print(F(",")); Serial.println(SimpleDHTErrDuration(err));
        }
        return;
    }
    // If alarm state was changed, broadcast it to the udp out
    if (checkForAlarm()) {
        #if DEBUG >= 1
        Serial.print("Alarm changed to: ");
        Serial.println(alarm_state);
        #endif
    }
    if (monitor) {
        printReading(out, temperature, humidity);
    }

    // Check for if we wish to log our reading
    // Assuming it is a big enough number that we
    // Shouldn't bother using the smaller number type above
    static unsigned long log_delay = 0;
    if (millis() > log_delay) {
        log_delay += 60000 * LOG_DELAY; // 1000ms * 60s * delay in minutes
        #if DEBUG >= 2
            Serial.println(F("DHT log being written."));
        #endif
        logReading();
    }
}

bool dht_control::checkForAlarm() {
    int temp = toFahrenheit(temperature);
    int8_t state_check;
    // First check what the current state is
    if (temp <= alarm_gates[0])
        state_check = -2;
    else if (temp <= alarm_gates[1])
        state_check = -1;
    else if (temp <= alarm_gates[2])
        state_check = 0;
    else if (temp <= alarm_gates[3])
        state_check = 1;
    else
        state_check = 2;
    // If this is a new state we take action
    if (state_check != alarm_state) {
        alarm_state = state_check;
        out.udpBegin();
        out.print(F("Arduino Alarm: Temperature "));
        switch (alarm_state) {
            case -2:
                out.print(F("Major Under"));
                LED.setRGBColor(RGB_ALARM_LIGHT, 40, 0, 40);
                break;
            case -1:
                out.print(F("Minor Under"));
                LED.setRGBColor(RGB_ALARM_LIGHT, 0, 0, 50);
                break;
            case 0:
                out.print(F("Comfortable"));
                LED.setRGBColor(RGB_ALARM_LIGHT, 0, 50, 0);
                break;
            case 1:
                out.print(F("Minor Over"));
                LED.setRGBColor(RGB_ALARM_LIGHT, 50, 25, 0);
                break;
            case 2:
                out.print(F("Major Over"));
                LED.setRGBColor(RGB_ALARM_LIGHT, 100, 0, 0);
                break;
        }
        if (alarm_state != 0)
            LED.setLightStatus(RGB_ALARM_LIGHT, t_BLINK);
        else
            LED.setLightStatus(RGB_ALARM_LIGHT, t_ON);
        out.print(F(" "));
        printReading(out, temperature, humidity);
        out.udpEnd();
        return true;
    }
    return false;
}

void dht_control::clearLog() {
    for (unsigned int i = 0; i < MAX_LOG_BYTES; i++)
        EEPROM.write(EEPROM_LOGS + i, 0);
    log_index = 2 * sizeof(int);
    log_entries = 0;
    // First couple bytes are the index and entries values
    EEPROM.write(EEPROM_LOGS, log_index);
    EEPROM.write(EEPROM_LOGS + sizeof(int), log_entries);
    out.println("DHT Log cleared.");
}

log_entry dht_control::getLogEntry(uint8_t i) {
    uint16_t memIndex;
    log_entry entry;
    // What is the current starting point of our logs?
    if (log_entries == MAX_LOG_ENTRIES)
        memIndex = log_index;
    else
        memIndex = 2 * sizeof(int);
    // Move along the memory i number of spots
    memIndex += (i * log_size);
    // If we are past max then start from the begining again
    if (memIndex + log_size > MAX_LOG_BYTES) {
        // Number of entries past the max we are at
        uint8_t overflow_i = (memIndex - MAX_LOG_BYTES) / log_size;
        memIndex = 2 * sizeof(int) + overflow_i * log_size;
    }
    #if DEBUG >= 1
    Serial.println(memIndex);
    #endif
    EEPROM.get(EEPROM_LOGS + memIndex, entry);
    return entry;
}

// Enter the current readings into the EEPROM log
void dht_control::logReading() {
    // Build the log
    log_entry newLog;
    newLog.ts = rtc_ptr->readTime();
    newLog.temp = temperature;
    newLog.humid = humidity;
    // Only change entries if we haven't hit max logs
    if (log_entries < MAX_LOG_ENTRIES) {
        log_entries++;
        EEPROM.write(EEPROM_LOGS + sizeof(int), log_entries);
    }
    #if DEBUG >= 3
    Serial.print(F("Current-entries max-entries log-index: "));
    Serial.print(log_entries); Serial.print(F(" "));
    Serial.print(MAX_LOG_ENTRIES); Serial.print(F(" "));
    Serial.print(log_index); Serial.println(F(" "));
    #endif
    // Write the log to memory
    EEPROM.put(EEPROM_LOGS + log_index, newLog);
    // Get the new index and write it to 0
    log_index += log_size;
    // Check if we need to start overwriting old logs
    if (log_index + log_size > MAX_LOG_BYTES) {
        log_index = 2 * sizeof(int);
    }
    EEPROM.write(EEPROM_LOGS, log_index);
}

// Print info and stats about the log 
void dht_control::printLogInfo() {
    out.print(log_entries);
    out.print(F(" entries stored in the log. Next memory address is "));
    out.println(log_index);
    if (log_entries == 0) return;
    log_entry entry;
    byte max_temp = 0, min_temp = 255, ct = 0;
    for (int i = 2 * sizeof(int); ct < log_entries; i+= log_size) {
        EEPROM.get(EEPROM_LOGS + i, entry);
        max_temp = max(max_temp, entry.temp);
        min_temp = min(min_temp, entry.temp);
        ct++;
    }
    out.print(F("Max Temperature: "));
    printTemperature(out, max_temp);
    out.print(F("\n"));
    out.print(F("Min Temperature: "));
    printTemperature(out, min_temp);
    out.print(F("\n"));
}

// Print the entirety of the logs
void dht_control::printLogs() {
    #if DEBUG >= 1
    logReading();
    #endif
    if (log_entries == 0) {
        Serial.println(F("No logs."));
        return;
    }
    #if DEBUG >= 1
    Serial.println(F("Log-index log-size log-entries max_entries: "));
    Serial.print(log_index); Serial.print(F(" "));
    Serial.print(log_size); Serial.print(F(" "));
    Serial.println(log_entries);
    #endif
    out.println(F("Date\tTime\tTemp, Humidity"));
    // Calculate index of the oldest entry
    uint8_t mem_index = 2 * sizeof(int);
    log_entry entry;
    // If this number of entries is at max our oldest entry
    //  will actually be the next one we plan to overwrite
    //  at log_index
    if (log_entries == MAX_LOG_ENTRIES) {
        mem_index = log_index;
    }
    for (unsigned int ct = 0; ct < log_entries; ct++) {
        entry = getLogEntry(ct);
        rtc_ptr->print(entry.ts);
        out.print(F("\t"));
        printReading(out, entry.temp,entry.humid);
    }
}

void dht_control::printReading(Print &Printer, float temp, float humid) {
    printTemperature(Printer, temp);
    Printer.print(F(", "));
    Printer.print((int)humid);
    Printer.println(F("%RH"));
}

// Print status of the DHT controller itself
void dht_control::printStatus(Print &Printer) {
    printReading(Printer, temperature, humidity);
    Printer.print(F("Scale set to "));
    Printer.println(isFahrenheit ? F("Fahrenheit"): F("Celcius"));
}

void dht_control::printTemperature(Print &Printer, float t) {
    if (isFahrenheit) {
        Printer.print((int)toFahrenheit(t));
        Printer.print(F("°F"));
    }
    else {
        Printer.print((int)t);
        Printer.print(F("°C"));
    }
}

void dht_control::setAlarmGates(int majL, int minL, int minH, int majH) {
    alarm_gates[0] = majL;
    alarm_gates[1] = minL;
    alarm_gates[2] = minH;
    alarm_gates[3] = majH;
    for(int i = 0; i < 4; i++) {
        EEPROM.put(EEPROM_ALARMS + i*sizeof(int), alarm_gates[i]);
    }
}

void dht_control::setToFahrenheit(bool f) {
    isFahrenheit = f;
}

int dht_control::toFahrenheit(float celcius) {
    return (celcius * 1.8) + 32;
}

void dht_control::toggleMonitor() {
    monitor = !monitor;
    if (monitor)
        Serial.println(F("DHT readings will now be printed."));
}