// network_control.h
/* Handles network send and receive functions */
#ifndef NETC_H
#define NETC_H

#include <Arduino.h>
#include <Ethernet.h>
#include <EEPROM.h>
#include "token_definitions.h"

class network_control
{
    private:
        unsigned long timer;
        byte mac_address[6] = {0xAA, 0x2B, 0xCC, 0x4D, 0xEE, 0x6F};
        IPAddress dest_ip{192, 168, 1, 1}; // Dest IP and port gets overwritten by first contact
        unsigned int local_port = 8888;
        unsigned int dest_port = 8888;
        unsigned int packetBufferSize;
        char packetBuffer[UDP_TX_PACKET_MAX_SIZE];
        bool active = true;
        bool dest_set = false;
    public:
        EthernetUDP UDP;
        IPAddress local_ip{192, 168, 1, 177};
        IPAddress subnet_addr{255, 255, 255, 255};
        IPAddress gateway_addr{255, 255, 255, 255};
        unsigned long packetsSent = 0, packetsRcvd = 0;

        // Arduino intial setup function
        void setup();

        // Arduino loop call receives commands over UDP
        //      When it gets a message it will return true and
        //      the packetBuffer can be processed
        bool loop();

        void beginPacket();
        bool connect();
        void endPacket();
        char* getPacketBuffer();
        unsigned int getPacketBufferLength();
        void setNetworkLight(bool);
        
        void saveDestAddrPort(IPAddress, unsigned int);
        void saveLocalIPAddr(IPAddress);
        void saveSubnetAddr(IPAddress);
        void saveGatewayAddr(IPAddress);
};

#endif