#include "Arduino.h"
#include "ioModbus.h"

#include <SPI.h>
#include <Ethernet.h>

#ifndef _IO_MODBUS_CLIENT_H
#define _IO_MODBUS_CLIENT_H

#define DEFAULT_MODBUSIP_PORT 502
#define SIZE_FRAME_MODBUS_IP 128

class ioModbusClient : public ioModbus
{

private:
    EthernetServer serverModbus;
    uint8_t MBAP[8] = {0};
    uint8_t frame[SIZE_FRAME_MODBUS_IP] = {0};

public:
    ioModbusClient() : serverModbus(DEFAULT_MODBUSIP_PORT), ioModbus(247){};
    ioModbusClient(uint8_t address, uint16_t port = DEFAULT_MODBUSIP_PORT) : serverModbus(port), ioModbus(address){};

    void begin(uint16_t port = DEFAULT_MODBUSIP_PORT);

    void poll();
};

void ioModbusClient::begin(uint16_t port = DEFAULT_MODBUSIP_PORT)
{
    serverModbus = EthernetServer(port);
    serverModbus.begin();
};

void ioModbusClient::poll()
{
    EthernetClient clientModbus = serverModbus.available();
    if (clientModbus)
    {
        if (clientModbus.connected())
        {
            int i = 0;
            while (clientModbus.available())
            {
                MBAP[i] = clientModbus.read();
                i++;
                if (i == 6)
                    break; //MBAP length has 7 bytes size
            }

            uint16_t sizeInput = MBAP[4] << 8 | MBAP[5];
            if (MBAP[2] != 0 || MBAP[3] != 0)
                return; //Not a MODBUSIP packet
            if (sizeInput > SIZE_FRAME_MODBUS_IP)
                return; //Length is over MODBUSIP_MAXFRAME

            i = 0;
            while (clientModbus.available())
            {
                frame[i++] = clientModbus.read();
                if (i == sizeInput)
                    break;
            }
            clientModbus.flush();
            printBuffer(frame, sizeInput);
            uint16_t sizeOutput = receivePDU(frame, sizeInput);
            printBuffer(frame, sizeOutput);
            if (sizeOutput != 0)
            {
                //MBAP
                MBAP[4] = (sizeOutput) >> 8; //_len+1 for last byte from MBAP
                MBAP[5] = (sizeOutput)&0x00FF;

                uint8_t sendbuffer[6 + sizeOutput];

                for (i = 0; i < 6; i++)
                {
                    sendbuffer[i] = MBAP[i];
                }
                //PDU Frame
                for (i = 0; i < sizeOutput; i++)
                {
                    sendbuffer[i + 6] = frame[i];
                }
                clientModbus.write(sendbuffer, sizeOutput + 6);
            }
        }
        if (!clientModbus.connected())
        {
            clientModbus.stop();
        }
    }
}

#endif