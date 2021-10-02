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

    uint16_t calculateCRC(uint8_t);

public:
    ioModbusSlave(HardwareSerial &SerialPort, int TxEnablePin=-1) : ioModbus(247), ioSerial(SerialPort, frame, BUFFER_SIZE, TxEnablePin){};
    ~ioModbusSlave(){};

    void init(uint8_t);
    void poll();
};

// PRIVATE

uint16_t ioModbusSlave::calculateCRC(uint8_t bufferSize)
{
    uint16_t temp, temp2, flag;
    temp = 0xFFFF;
    for (uint8_t i = 0; i < bufferSize; i++)
    {
        temp = temp ^ frame[i];
        for (uint8_t j = 1; j <= 8; j++)
        {
            flag = temp & 0x0001;
            temp >>= 1;
            if (flag)
                temp ^= 0xA001;
        }
    }
    // Reverse byte order.
    temp2 = temp >> 8;
    temp = (temp << 8) | temp2;
    temp &= 0xFFFF;
    // the returned value is already swapped
    // crcLo byte is first & crcHi byte is last
    return temp;
};

// PUBLIC
void ioModbusSlave::init(uint8_t address){
    setAddress(address);
}

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