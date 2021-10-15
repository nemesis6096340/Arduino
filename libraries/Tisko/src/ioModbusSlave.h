//#define _DEBUG_IO_MODBUS

#include "Arduino.h"
#include "ioModbus.h"
#include "ioSerial.h"

#ifndef _IO_MODBUS_SLAVE_H
#define _IO_MODBUS_SLAVE_H

#define BUFFER_SIZE 128

class ioModbusSlave : public ioModbus, public ioSerial
{
private:
    uint8_t frame[BUFFER_SIZE];
    uint16_t errorCount;

public:
    //ioModbusSlave(HardwareSerial &SerialPort, int TxEnablePin=-1) : ioModbus(247), ioSerial(SerialPort, frame, BUFFER_SIZE, TxEnablePin){};
    ioModbusSlave() : ioModbus(247), ioSerial(frame, (uint16_t)BUFFER_SIZE){};
    ~ioModbusSlave(){};

    void init(uint8_t);
    void poll();
};

void ioModbusSlave::poll()
{
    int buffer = readPacket();
    if (buffer > 7)
    {
        uint8_t id = frame[0];
        if (id == address) // if the recieved ID matches the slaveID or broadcasting id (0), continue
        {
            printBuffer(frame, buffer);
            uint16_t crc = ((frame[buffer - 2] << 8) | frame[buffer - 1]); // combine the crc Low & High bytes
            if (calculateCRC(buffer - 2) == crc)                           // if the calculated crc matches the recieved crc continue
            {
                uint16_t sizeOutput = receivePDU(frame, buffer);
                uint16_t crc16 = calculateCRC(sizeOutput);
                frame[sizeOutput++] = crc16 >> 8; // split crc into 2 bytes
                frame[sizeOutput++] = crc16 & 0xFF;
                sendPacket(sizeOutput);
            }
            else // checksum failed
                errorCount++;
        } // incorrect id
    }
    else if (buffer > 0 && buffer < 8)
        errorCount++; // corrupted packet
};
#endif