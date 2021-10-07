
#define _DEBUG_IO_MODBUS

#include <ioModbusClient.h>

ioModbusClient modbusIP(7, 502);

byte mac[] = {0x90, 0xA2, 0xDA, 0xAC, 0x00, 0x00};
IPAddress ip(10, 0, 115, 7);
IPAddress myDns(10, 0, 115, 1);

uint16_t registers[6] = {100, 425, 30, 1727, 0, 256};

uint8_t discretInput[1] = {0b00000011};

const uint8_t pinMask_DIN[] = {2, 3};
const uint8_t pinMask_DOUT[] = {7, 8};
const uint8_t pinMask_AIN[] = {A0, A1, A2, A3};
const uint8_t pinMask_AOUT[] = {5, 6};

const uint8_t NUM_DISCRETE_INPUT = sizeof(pinMask_DIN);
const uint8_t NUM_DISCRETE_COILS = sizeof(pinMask_DOUT);
const uint8_t NUM_INPUT_REGISTERS = sizeof(pinMask_AIN);
const uint8_t NUM_HOLDING_REGISTERS = sizeof(pinMask_AOUT);

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
    configurePins();

    modbusIP.begin(123);
    modbusIP.setAddress(7);

    
    modbusIP.addDiscretsInput(100, discretInput, 1);
    modbusIP.addHoldingRegisters(10, registers, 6);
}

void loop()
{
    modbusIP.poll();
    update();

}

void configurePins()
{
    for (int i = 0; i < NUM_DISCRETE_INPUT; i++)
    {
        pinMode(pinMask_DIN[i], INPUT_PULLUP);
    }
    
    for (int i = 0; i < NUM_DISCRETE_COILS; i++)
    {
        pinMode(pinMask_DOUT[i], OUTPUT);
    }

    for (int i = 0; i < NUM_INPUT_REGISTERS; i++)
    {
        pinMode(pinMask_AIN[i], INPUT);
    }
        
    for (int i = 0; i < NUM_HOLDING_REGISTERS; i++)
    {
        pinMode(pinMask_AOUT[i], OUTPUT);
    }
}

void update(){
    for (int i = 0; i < NUM_DISCRETE_INPUT; ++i) 
    {   uint8_t idx = i / 8;
        bitWrite(
            discretInput[i/8],
            i%8,
            digitalRead(pinMask_DIN[i])
        );
    }
}