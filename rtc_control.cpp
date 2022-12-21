// rtc_control.cpp
#include "rtc_control.h"

#define DEBUG 0

extern Output out;
extern network_control NET;

DateTime rtc_control::readTime() {
    return Clock.read();
}

void rtc_control::printStatus() {
    out.print(F("Date (yyyy/mm/dd): "));
    if (out.udp_print) {
        Clock.printDateTo_YMD(NET.UDP);
        out.print(F("\n\rTime (hh:mm:ss): "));
        Clock.printTimeTo_HMS(NET.UDP);
    }
    else {
        Clock.printDateTo_YMD(Serial);
        out.print(F("\n\rTime (hh:mm:ss): "));
        Clock.printTimeTo_HMS(Serial);
    }
}

void rtc_control::setup() {
    Clock.begin();
}

bool rtc_control::setDate(byte yy, byte mm, byte dd) {
    DateTime ts = Clock.read();
    ts.Day    = (uint8_t)dd;
    ts.Month  = (uint8_t)mm;
    ts.Year   = (uint8_t)yy;
    bool valid = true;
    if (ts.Year > 99 || ts.Month > 12 ||
            ts.Year < 1 || ts.Month < 1 || ts.Day < 1)
        valid = false;
    switch (ts.Month) {
        case 12:
        case 4:
        case 6:
        case 9:
        case 11:
            if (ts.Day > 30)
                valid = false;
            break;
        case 2:
            if (ts.Day > 29 || (ts.Year % 4 != 0 && ts.Day > 28))
                valid = false;
            break;
        default:
            if (ts.Day > 31)
                valid = false;
    }
    if (valid) {
        Clock.write(ts);
        out.print(F("Date-Time set to "));
    }
    else
        out.print(F("Invalid Date-Time: "));
    print(ts);
    out.print(F("\n"));
    return valid;
}

bool rtc_control::setTime(byte hh, byte mm, byte ss) {
    DateTime ts = Clock.read();
    ts.Second    = (uint8_t)ss;
    ts.Minute  = (uint8_t)mm;
    ts.Hour   = (uint8_t)hh;
    bool valid = true;
    if (ts.Second > 59 || ts.Minute > 59 || ts.Hour > 24)
        valid = false;
    if (valid) {
        Clock.write(ts);
        out.print(F("Time set to "));
    }
    else
        out.print(F("Invalid Date-Time: "));
    print(ts);
    out.print(F("\n"));
    return valid;
}

void rtc_control::print(const DateTime &ts) {
    if (out.udp_print) {
        Clock.printDateTo_YMD(NET.UDP, ts);
        out.print(F(" "));
        Clock.printTimeTo_HMS(NET.UDP, ts);
    }
    else {
        Clock.printDateTo_YMD(Serial, ts);
        out.print(F(" "));
        Clock.printTimeTo_HMS(Serial, ts);
    }
}