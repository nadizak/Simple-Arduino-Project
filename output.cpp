// output.cpp
#include "output.h"

#define DEBUG 0

extern network_control NET;

size_t Output::write(uint8_t p) {
    if (udp_print)
        return NET.UDP.write(p);
    else
        return Serial.write(p);
}

void Output::udpBegin() {
    NET.beginPacket();
    udp_print = true;
}

void Output::udpEnd() {
    NET.endPacket();
    udp_print = false;
}