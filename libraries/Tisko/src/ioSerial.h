#ifndef _IO_SERIAL_h
#define _IO_SERIAL_h

#include "Arduino.h"

class ioSerial
{
protected:
    HardwareSerial *SerialPort;

private:
    uint8_t *Frame;
    uint16_t FrameSize;

    int TxEnablePin;

    uint16_t T1_5; // inter character time out
    uint16_t T3_5; // frame delay

public:
    ioSerial(HardwareSerial &SerialPort, uint8_t *Frame, uint16_t FrameSize, int TxEnablePin = -1);
    ~ioSerial();

    void begin(uint32_t baudrate, uint8_t byteformat = SERIAL_8N1);
    void sendPacket(uint8_t bufferSize);
    void setTimeout(uint16_t T1_5, uint16_t T3_5);
    void clear();
    void flush();

    uint16_t readPacket();
};

ioSerial::ioSerial(HardwareSerial &SerialPort, uint8_t *Frame, uint16_t FrameSize, int TxEnablePin=-1)
{
    this->SerialPort = &SerialPort;
    this->Frame = Frame;
    this->FrameSize = FrameSize;
    this->TxEnablePin = TxEnablePin;
};

ioSerial::~ioSerial(){};

void ioSerial::begin(uint32_t BaudRate, uint8_t ByteFormat)
{
#ifdef __arm__
    SerialPort->begin(BaudRate);
#else
    SerialPort->begin(BaudRate, ByteFormat);
#endif

    if (BaudRate > 19200)
    {
        T1_5 = 750;
        T3_5 = 1750;
    }
    else
    {
        T1_5 = 15000000 / BaudRate; // 1T * 1.5 = T1.5
        T3_5 = 35000000 / BaudRate; // 1T * 3.5 = T3.5
    }

    if (TxEnablePin != -1)
    {
        pinMode(TxEnablePin, OUTPUT);
        digitalWrite(TxEnablePin, LOW);
    }

    this->clear();
};

void ioSerial::setTimeout(uint16_t T1_5, uint16_t T3_5)
{
    this->T1_5 = T1_5;
    this->T3_5 = T3_5;
};

void ioSerial::clear()
{
    for (int i = 0; i < FrameSize; i++)
        Frame[i] = 0;
    this->flush();
};

void ioSerial::flush()
{
    SerialPort->flush();

    uint32_t _tmr = millis();
    while (SerialPort->available() > 0)
    {
        if ((millis() - _tmr) > 400UL)
        { // Reading... Waiting... But not forever......
            break;
        }
        SerialPort->read();
        delayMicroseconds(T1_5);
    }
};

uint16_t ioSerial::readPacket()
{
    if (SerialPort->available())
    {
        uint16_t buffer = 0;
        uint8_t overflow = 0;

        while (SerialPort->available())
        {
            // If more bytes is received than the BUFFER_SIZE the overflow flag will be set and the
            // serial buffer will be red untill all the data is cleared from the receive buffer.
            if (overflow)
                SerialPort->read();
            else
            {
                if (buffer == FrameSize)
                    overflow = 1;
                Frame[buffer] = SerialPort->read();
                buffer++;
            }
            delayMicroseconds(T1_5); // inter character time out
        }

        // If an overflow occurred increment the errorCount
        // variable and return to the main sketch without
        // responding to the request i.e. force a timeout
        if (overflow)
            return FrameSize;
        return buffer;
    }
    return 0;
};

void ioSerial::sendPacket(uint8_t packetSize)
{
    if (TxEnablePin != -1)
        digitalWrite(TxEnablePin, HIGH);

    for (uint8_t i = 0; i < packetSize; i++)
        SerialPort->write(Frame[i]);

    SerialPort->flush();

    // allow a frame delay to indicate end of transmission
    delayMicroseconds(T3_5);

    if (TxEnablePin != -1)
        digitalWrite(TxEnablePin, LOW);
};

#endif