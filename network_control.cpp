// network_control.cpp
#include "network_control.h"
#include "led_control.h"

#define DEBUG 0

#define CS_PIN 10
#define UDP_DELAY 5000
#define NETWORK_SAVE_START 247
#define RGB_NETWORK_CONNECTED_LIGHT 3
// EEPROM saved from 247 through 272
/*  dest_ip ..... 6
    dest_port ... 2
    local_ip .... 6
    subnet_ip ... 6
    gateway ..... 6
*/

// Used to toggle network connected / not connected
extern led_control LED;

void network_control::setup() {
    Ethernet.init(CS_PIN);

    // Check for saved destination ip:port
    EEPROM.get(NETWORK_SAVE_START, dest_ip);
    EEPROM.get(NETWORK_SAVE_START + sizeof(IPAddress), dest_port);
    // Check for subnet and gateway
    EEPROM.get(NETWORK_SAVE_START + sizeof(IPAddress) + sizeof(int),
            local_ip);
    EEPROM.get(NETWORK_SAVE_START + 2*sizeof(IPAddress) + sizeof(int),
            subnet_addr);
    EEPROM.get(NETWORK_SAVE_START + 3*sizeof(IPAddress) + sizeof(int),
            gateway_addr);
    #if DEBUG >= 1
    Serial.print(F("Dest IP and Port pulled from EEPROM: "));
    for (int i=0; i < 4; i++) {
        Serial.print(dest_ip[i], DEC);
        if (i < 3) {
            Serial.print(F("."));
        }
    }
    Serial.print(F(":"));
    Serial.println(dest_port);
    Serial.print(F("Local IP pulled from EEPROM: "));
    for (int i=0; i < 4; i++) {
        Serial.print(local_ip[i], DEC);
        if (i < 3) {
            Serial.print(F("."));
        }
    }
    Serial.println();
    Serial.print(F("Subnet pulled from EEPROM: "));
    for (int i=0; i < 4; i++) {
        Serial.print(subnet_addr[i], DEC);
        if (i < 3) {
            Serial.print(F("."));
        }
    }
    Serial.println();
    Serial.print(F("Gateway pulled from EEPROM: "));
    for (int i=0; i < 4; i++) {
        Serial.print(gateway_addr[i], DEC);
        if (i < 3) {
            Serial.print(F("."));
        }
    }
    Serial.println();
    #endif

    Ethernet.begin(mac_address, local_ip);
    active = false;
    // Connection will start in loop call
}

bool network_control::loop() {
    // Check timer
    if (millis() - timer < 0)
        return false;
    
    timer = millis() + UDP_DELAY;

    if (active && Ethernet.linkStatus() == LinkOFF) {
        // Cable disconnected?
        active = false;
        setNetworkLight(false);
    }

    if (!active && Ethernet.linkStatus() == LinkON) {
        // The connection is back...?
        if (connect()) {
            active = true;
            setNetworkLight(true);
        }
        else {
            // Wait longer to check again
            timer = millis() + UDP_DELAY * 10;
        }
    }

    if (!active) return false;

    // if there's data available, read a packet
    int packetSize = UDP.parsePacket();
    if (active && packetSize) {
        #if DEBUG >= 1
        Serial.print(F("Received packet of size "));
        Serial.println(packetSize);
        Serial.print(F("From "));
        IPAddress remote = UDP.remoteIP();
        for (int i=0; i < 4; i++) {
            Serial.print(remote[i], DEC);
            if (i < 3) {
                Serial.print(F("."));
            }
        }
        Serial.print(F(", port "));
        Serial.println(UDP.remotePort());
        #endif

        if (!dest_set) {
            dest_set = true;
            saveDestAddrPort(UDP.remoteIP(), UDP.remotePort());
            #if DEBUG >= 1
            Serial.println("Address saved to EEPROM");
            #endif
        }

        // Clear out any potential data in the buffer
        for (int i=0; i< UDP_TX_PACKET_MAX_SIZE; i++)
            packetBuffer[i] = '\0';
        // read the packet into packetBufffer
        UDP.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
        #if DEBUG >= 1
        Serial.println(F("Contents:"));
        Serial.println(packetBuffer);
        #endif

        // Save length and return true to signal ready to be processed
        packetBufferSize = packetSize;
        packetsRcvd++;
        return true;
    }
    return false;
}

void network_control::beginPacket()  {
    #if DEBUG >= 2
    Serial.println("Begin UDP packet send.");
    Serial.println(active? "Active": "Not Active");
    #endif
    if (active)
        UDP.beginPacket(dest_ip, dest_port);
}

void network_control::endPacket() {
    #if DEBUG >= 2
    Serial.println("Ending UDP packet send.");
    Serial.println(active? "Active": "Not Active");
    #endif
    if (active) {
        UDP.endPacket();
        packetsSent++;
    }
}

bool network_control::connect() {
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware ||
            Ethernet.linkStatus() == LinkOFF) {
        #if DEBUG >= 1
        Serial.println(F("Ethernet cable is not connected or shield not found."));
        #endif
        setNetworkLight(false);
        return false;
    }
    else {
        // start UDP
        UDP.begin(local_port);
        setNetworkLight(true);
        return true;
    }
}

char* network_control::getPacketBuffer() {
    return packetBuffer;
}

unsigned int network_control::getPacketBufferLength() {
    return packetBufferSize;
}

void network_control::setNetworkLight(bool on) {
    LED.setRGBColor(RGB_NETWORK_CONNECTED_LIGHT, on? 0: 50, on? 50: 0, 0);
}

// EEPROM saving functions
/*  dest_ip ..... 6
    dest_port ... 1
    local_ip .... 6
    subnet_ip ... 6
    gateway ..... 6
*/
void network_control::saveDestAddrPort(IPAddress ip, unsigned int port) {
    EEPROM.put(NETWORK_SAVE_START, ip);
    EEPROM.put(NETWORK_SAVE_START + sizeof(IPAddress), port);
    dest_ip = ip;
    dest_port = port;
    #if DEBUG >= 2
    Serial.print(F("new Dest IP and Port saved to EEPROM: "));
    for (int i=0; i < 4; i++) {
        Serial.print(dest_ip[i], DEC);
        if (i < 3) {
            Serial.print(F("."));
        }
    }
    Serial.print(F(":"));
    Serial.println(dest_port);
    #endif
}
void network_control::saveLocalIPAddr(IPAddress ip) {
    EEPROM.put(NETWORK_SAVE_START + sizeof(IPAddress) + sizeof(int), ip);
    local_ip = ip;
    // Do we need to close connection and restart...?
}
void network_control::saveSubnetAddr(IPAddress ip) {
    EEPROM.put(NETWORK_SAVE_START + 2*sizeof(IPAddress) + sizeof(int), ip);
    subnet_addr = ip;
}
void network_control::saveGatewayAddr(IPAddress ip) {
    EEPROM.put(NETWORK_SAVE_START + 3*sizeof(IPAddress) + sizeof(int), ip);
    gateway_addr = ip;
}