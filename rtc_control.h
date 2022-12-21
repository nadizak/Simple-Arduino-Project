// rtc_control.h
/* Class to wrap around the RTC time controller */
#ifndef RTC_H
#define RTC_H

#include <Arduino.h>
#include <DS3231_Simple.h>
#include "token_definitions.h"
#include "output.h"

class rtc_control
{
    private:
        DS3231_Simple Clock;
    public:
        // Wraps the Clock's read function
        DateTime readTime();
        
        // Just prints current time
        void printStatus();

        // Just outputs current time/date
        void setup();

        // Set the date, return true if succeeded
        bool setDate(byte, byte, byte);

        // Set the time, return true if succeeded
        bool setTime(byte, byte, byte);

        // Print using the external Output object
        void print(const DateTime&);
};

#endif