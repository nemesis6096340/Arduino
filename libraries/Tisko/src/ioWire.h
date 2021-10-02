#ifndef _IO_WIRE_h
#define _IO_WIRE_h

#include <Arduino.h>
#include <Wire.h>

class ioWire
{
public:
    void begin(uint8_t *registers, uint8_t address)
    {
        this->registers = registers;
        this->address = address;
        Wire.begin();
    }
    uint8_t readRegister(uint8_t index);
    void writeRegister(uint8_t index, uint8_t value);

    void writeRegisters(uint8_t index, uint8_t size);
    void readRegisters(uint8_t index, uint8_t size);

private:
    byte *registers;
    byte address;
};

/************************************************************************
 *  
 ***********************************************************************/
uint8_t ioWire::readRegister(uint8_t index)
{
    uint8_t readValue = 0;
    Wire.beginTransmission(address);
    Wire.write(index);
    Wire.endTransmission();

    Wire.requestFrom(address, 1, true);
    if (Wire.available())
    {
        readValue = Wire.read();
    }
    return readValue;
};

void ioWire::writeRegister(uint8_t index, uint8_t value)
{
    Wire.beginTransmission(address);
    Wire.write(index);
    Wire.write(value);
    Wire.endTransmission();
};

void ioWire::writeRegisters(uint8_t index, uint8_t size)
{
    Wire.beginTransmission(address);
    Wire.write(index);
    for (int i = index; i < index + size; i++)
    {
        Wire.write(registers[i]);
    }
    Wire.endTransmission();
};

void ioWire::readRegisters(uint8_t index, uint8_t size)
{
    // set position for read
    Wire.beginTransmission(address);
    Wire.write(index);
    Wire.endTransmission();

    // read from address
    uint8_t len = Wire.requestFrom(address, size, true);
    uint8_t pos = index;
    while (Wire.available())
    {
        this->registers[pos++] = (byte)Wire.read();
    }
};

#endif