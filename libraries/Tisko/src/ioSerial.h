#ifndef _IO_SERIAL_h
#define _IO_SERIAL_h

#include "Arduino.h"
#ifdef USE_SOFTWARE_SERIAL
#include <SoftwareSerial.h>
#endif
class ioSerial
{
protected:
    //HardwareSerial *Port;
    Stream* Port;

private:
    uint8_t *Frame;
    uint16_t Size;

    int TxEnablePin;

    uint16_t T1_5; // inter character time out
    uint16_t T3_5; // frame delay

    void setTimeout(uint32_t);

public:
    ioSerial(uint8_t *Frame, uint16_t Size);
    ~ioSerial();

    void config(HardwareSerial &Port, uint32_t BaudRate, uint8_t ByteFormat=SERIAL_8N1, int TxEnablePin = -1);
#ifdef __AVR_ATmega32U4__
    void config(Serial_ &Port, uint32_t BaudRate, uint8_t ByteFormat=SERIAL_8N1, int TxEnablePin = -1);
#endif
#ifdef USE_SOFTWARE_SERIAL
    void config(SoftwareSerial &Port, uint32_t BaudRate, int TxEnablePin = -1);
#endif

    void sendPacket(uint8_t);
    void setTimeout(uint16_t T1_5, uint16_t T3_5);
    void start();
    void clear();
    void flush();

    uint16_t readPacket();
};

ioSerial::ioSerial(uint8_t *Frame, uint16_t Size)
{
    this->Frame = Frame;
    this->Size = Size;
};

ioSerial::~ioSerial(){};

void ioSerial::config(HardwareSerial &Port, uint32_t BaudRate, uint8_t ByteFormat, int TxEnablePin){    
    this->Port = &Port;
    this->TxEnablePin = TxEnablePin;
    if(ByteFormat == SERIAL_8N1)
        (&Port)->begin(BaudRate);
    else
        (&Port)->begin(BaudRate, ByteFormat);

    this->setTimeout(BaudRate);
    this->start();
    this->clear();
}
#ifdef __AVR_ATmega32U4__
void ioSerial::config(Serial_ &Port, uint32_t BaudRate, uint8_t ByteFormat, int TxEnablePin) {    
    this->Port = &Port;
    this->TxEnablePin = TxEnablePin;
    if(ByteFormat == SERIAL_8N1)
        (&Port)->begin(BaudRate);
    else
        (&Port)->begin(BaudRate, ByteFormat);

    while (!(&Port));

    this->setTimeout(BaudRate);
    this->start();
    this->clear();
}
#endif 
#ifdef USE_SOFTWARE_SERIAL
void ioSerial::config(SoftwareSerial &Port, uint32_t BaudRate, int TxEnablePin) {
    this->Port = &Port;
    this->TxEnablePin = TxEnablePin;
    (&Port)->begin(BaudRate);

    this->setTimeout(BaudRate);
    this->start();
    this->clear();
}
#endif

void ioSerial::setTimeout(uint32_t BaudRate)
{
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
}

void ioSerial::start()
{
    delay(200);
    if (TxEnablePin != -1)
    {
        pinMode(TxEnablePin, OUTPUT);
        digitalWrite(TxEnablePin, LOW);
    }
    this->clear();
    this->flush();
}

void ioSerial::setTimeout(uint16_t T1_5, uint16_t T3_5)
{
    this->T1_5 = T1_5;
    this->T3_5 = T3_5;
};

void ioSerial::clear()
{
    for (int i = 0; i < Size; i++)
        Frame[i] = 0;
    this->flush();
};

void ioSerial::flush()
{
    Port->flush();

    uint32_t _tmr = millis();
    while (Port->available() > 0)
    {
        if ((millis() - _tmr) > 400UL)
        { // Reading... Waiting... But not forever......
            break;
        }
        Port->read();
        delayMicroseconds(T1_5);
    }
};

uint16_t ioSerial::readPacket()
{
    if (Port->available())
    {
        uint16_t buffer = 0;
        uint8_t overflow = 0;

        while (Port->available())
        {
            // If more bytes is received than the BUFFER_SIZE the overflow flag will be set and the
            // serial buffer will be red untill all the data is cleared from the receive buffer.
            if (overflow)
                Port->read();
            else
            {
                if (buffer == Size)
                    overflow = 1;
                Frame[buffer] = Port->read();
                buffer++;
            }
            delayMicroseconds(T1_5); // inter character time out
        }

        // If an overflow occurred increment the errorCount
        // variable and return to the main sketch without
        // responding to the request i.e. force a timeout
        if (overflow)
            return Size;
        return buffer;
    }
    return 0;
};

void ioSerial::sendPacket(uint8_t packetSize)
{
    if (TxEnablePin != -1)
        digitalWrite(TxEnablePin, HIGH);

    for (uint8_t i = 0; i < packetSize; i++)
        Port->write(Frame[i]);

    Port->flush();

    // allow a frame delay to indicate end of transmission
    delayMicroseconds(T3_5);

    if (TxEnablePin != -1)
        digitalWrite(TxEnablePin, LOW);
};

#endif