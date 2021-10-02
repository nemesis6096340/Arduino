
#define _DEBUG_IO_MODBUS

#include <ioModbusClient.h>

ioModbusClient modbusIP(502);

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 0, 177);
IPAddress myDns(192, 168, 0, 1);

uint16_t registers[6] = {100, 425, 30, 1727, 0, 256};

void initEthernet()
{
    pinMode(9, OUTPUT);
    digitalWrite(9, LOW);
    delay(10);
    digitalWrite(9, HIGH);

    Ethernet.init(10);
    // start the Ethernet connection:
    Serial.println("Initialize Ethernet with DHCP:");
    if (Ethernet.begin(mac) == 0)
    {
        Serial.println("Failed to configure Ethernet using DHCP");
        // Check for Ethernet hardware present
        if (Ethernet.hardwareStatus() == EthernetNoHardware)
        {
            Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
            while (true)
            {
                delay(1); // do nothing, no point running without Ethernet hardware
            }
        }
        if (Ethernet.linkStatus() == LinkOFF)
        {
            Serial.println("Ethernet cable is not connected.");
        }
        // try to congifure using IP address instead of DHCP:
        Ethernet.begin(mac, ip, myDns);
    }
    else
    {
        Serial.print("  DHCP assigned IP ");
        Serial.println(Ethernet.localIP());
    }
    // give the Ethernet shield a second to initialize:
    delay(1000);
}

void setup()
{
    Serial.begin(115200);
    initEthernet();

    modbusIP.begin(1583);
    modbusIP.setAddress(7);
    modbusIP.addHoldingRegisters(10, registers, 6);
}

void loop()
{
    modbusIP.poll();
}
