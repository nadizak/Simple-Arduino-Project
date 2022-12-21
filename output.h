// output.h
/* Wraps around Serial print functions and UDP send functions
        to send output to correct location. */
#ifndef OUTPUT_H
#define OUTPUT_H

#include <Arduino.h>
#include "network_control.h"
#include "token_definitions.h"

class Output : public Print
{
        public:
                bool udp_print = false;

                void udpBegin();
                void udpEnd();

                virtual size_t write(uint8_t);
                using Print::write;
};

#endif