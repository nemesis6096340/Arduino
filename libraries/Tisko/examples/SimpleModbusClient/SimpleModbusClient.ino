
#define _DEBUG_IO_MODBUS

#include <ioModbusClient.h>

ioModbusClient modbusIP(502);

byte mac[] = {0x90, 0xA2, 0xDA, 0xAC, 0x00, 0x00};
IPAddress ip(192, 168, 0, 177);
IPAddress myDns(192, 168, 0, 1);

uint16_t registers[6] = {100, 425, 30, 1727, 0, 256};

uint8_t discrets[3] = {0b11110000,
                       0b11001100,
                       0b10101010};

uint8_t pinMask_DIN[] = {22, 24, 26, 28, 30, 32, 34, 36};
uint8_t pinMask_DOUT[] = {23, 25, 27, 29, 31, 33, 35, 37};
uint8_t pinMask_AIN[] = {A0, A1, A2, A3, A4, A5, A6, A7};
uint8_t pinMask_AOUT[] = {5, 6, 7, 8, 11, 12};

/*
uint8_t pinMask_DIN[] = {2, 3};
uint8_t pinMask_DOUT[] = {5,6};
uint8_t pinMask_AIN[] = {A0, A1, A2, A3};
uint8_t pinMask_AOUT[] = {7,8};
*/
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

    modbusIP.begin(123);
    modbusIP.setAddress(7);

    modbusIP.addDiscretsCoil(500, discrets, 3);
    modbusIP.addHoldingRegisters(10, registers, 6);
}

void loop()
{
    modbusIP.poll();
}
