#include "Arduino.h"
#include "Modbus.h"
#include <SPI.h>
#include <Ethernet.h>

#ifndef _MODBUS_CLIENT_H
#define _MODBUS_CLIENT_H

#define MODBUSIP_PORT 502
#define SIZE_BUFFER 64

#define MODBUSIP_MAXFRAME 200

class ModbusClient : public Modbus
{
   
private:
    EthernetServer _server;
    byte MBAP[8] = {0};
    byte outputData[SIZE_BUFFER] = {0};
    byte inputData[SIZE_BUFFER] = {0};

public:
    ModbusClient(): _server(MODBUSIP_PORT), Modbus(247){};
    ModbusClient(uint8_t address, uint16_t port = MODBUSIP_PORT): _server(port), Modbus(address){};
    
    void begin(){
        _server.begin();
    };

    void poll();
};

void ModbusClient::poll()
{
    EthernetClient _client = _server.available();
    if (_client)
    {
        if (_client.connected())
        {
            int i = 0;
            while (_client.available())
            {
                MBAP[i] = _client.read();
                //Serial.println(MBAP[i], HEX);
                i++;
                if (i == 6)
                    break; //MBAP length has 7 bytes size
            }

            word sizeInput = MBAP[4] << 8 | MBAP[5];
            //Serial.println(sizeInput);
            if (MBAP[2] != 0 || MBAP[3] != 0)
                return; //Not a MODBUSIP packet
            if (sizeInput > MODBUSIP_MAXFRAME)
                return; //Length is over MODBUSIP_MAXFRAME

            i = 0;
            while (_client.available())
            {
                inputData[i] = _client.read();
                //Serial.println(inputData[i], HEX);
                i++;
                if (i == sizeInput)
                    break;
            }
            _client.flush();
            word sizeOutput = receivePDU(inputData, sizeInput, outputData);
            if (sizeOutput != 0)
            {
                //MBAP
                MBAP[4] = (sizeOutput) >> 8; //_len+1 for last byte from MBAP
                MBAP[5] = (sizeOutput)&0x00FF;

                byte sendbuffer[6 + sizeOutput];

                for (i = 0; i < 6; i++)
                {
                    sendbuffer[i] = MBAP[i];
                }
                //PDU Frame
                for (i = 0; i < sizeOutput; i++)
                {
                    sendbuffer[i + 6] = outputData[i];
                }
                /*
              for (byte i = 0; i < sizeOutput + 6; i++) {
              char out[2];
              sprintf(out, "%02X", sendbuffer[i]);
              Serial.print(out);
              }*/
                //Serial.println();
                _client.write(sendbuffer, sizeOutput + 6);
            }
        }
        if (!_client.connected())
        {
            _client.stop();
        }
    }
}

#endif