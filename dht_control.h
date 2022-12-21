// dht_control.h
/* Class to control the DHT sensor */
#ifndef DHT_H
#define DHT_H

#include <Arduino.h>
#include <SimpleDHT.h>
#include <EEPROM.h>
#include "output.h"
#include "rtc_control.h"
#include "token_definitions.h"

struct log_entry {
    DateTime ts;
    char temp;
    byte humid;
    /* Might be better to not bother storing year?
            We can only store max of 113 entries with this size entry (9)
        which comes to 28.5 hours of logs if saving every 15 min.
            Assuming year is an extra byte we could have a max of
        31.8 hours. If we dropped day of week, 36.4 hours. And we if dropped the
        entire date portion and instead just recorded day of week, 42.5 hours.
        And if only stored time we could get 51 hours.
            All of these numbers are assuming we use the entire 1024 memory by doing
        the following:
            If we changed the num_entries byte to be a bit for whether we
        are overwriting older values. And then used the two bytes (minus that bit)
        to store a higher index without having to use another byte.
    */
};

class dht_control
{
    private:
        SimpleDHT22 dht22;
        static const uint16_t log_size = sizeof(log_entry);
        unsigned int log_index = 0, log_entries = 0;
        signed int alarm_state:3;
        rtc_control *rtc_ptr;
        bool isFahrenheit = true;
    public:
        signed int alarm_gates[4];
        bool monitor = false;
        float temperature = 0, humidity = 0;

        // Arduino setup function
        void setup(rtc_control*);

        // Looping portion, controls reads and logging calls and
        //  makes periodic calls to check for alarm.
        void loop();

        /* Checks currently stored data against "alarm conditions" and
            stores result in alarm_state. Set alarm LED's color and send
            out a single UDP message when alarm changes as well.
            __Desc____________State___Color
            Major Under <60  |  -2|purple
            Minor Under 61-70|  -1|blue
            Comfortable 71-80|   0|green
            Minor Over 81-90 |  +1|orange
            Major Over >90   |  +2|red
        */
        bool checkForAlarm();

        // Erases the portion of memory the controller uses
        void clearLog();

        // Retrieve a specific log object
        log_entry getLogEntry(uint8_t);

        // Get number of log entries
        uint8_t getEntriesCount() { return log_entries; }

        // Write current reading to the EEPROM
        void logReading();

        // Prints information about what is written to EEPROM
        void printLogInfo();

        // Print all of the logs written to EPROM
        void printLogs();

        // Print the temperature and humidity given
        void printReading(Print&, float, float);

        // Print current status of the controller
        void printStatus(Print&);

        // Print given temperature in F or C
        void printTemperature(Print&, float);

        // Save temperature gates to EEPROM and set them
        void setAlarmGates(int, int, int, int);

        // Set the controllers scale setting to F if true otherwise C
        void setToFahrenheit(bool);

        // Convert a C temp to F
        int toFahrenheit(float);

        // Just toggles whether the monitor flag is active or not and prints
        void toggleMonitor();
};

#endif