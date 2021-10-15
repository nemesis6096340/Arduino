#include "Arduino.h"
#include "ioModbus.h"
#include "ioSerial.h"

#ifndef _IO_MODBUS_MASTER_H
#define _IO_MODBUS_MASTER_H

#define BUFFER_SIZE 128

enum
{
    COM_IDLE = 0,
    COM_WAITING = 1

} COM_STATES;

enum
{
    ERR_NOT_MASTER = -1,
    ERR_POLLING = -2,
    ERR_BUFF_OVERFLOW = -3,
    ERR_BAD_CRC = -4,
    ERR_EXCEPTION = -5
} ERR_LIST;

const unsigned char functionSupported[] = {
    MODBUS_FUNCTION_READ_COILS,
    MODBUS_FUNCTION_READ_DISCRETE_INPUTS,
    MODBUS_FUNCTION_READ_HOLDING_REGISTERS,
    MODBUS_FUNCTION_READ_INPUT_REGISTERS,
    MODBUS_FUNCTION_WRITE_SINGLE_COIL,
    MODBUS_FUNCTION_WRITE_SINGLE_REGISTER,
    MODBUS_FUNCTION_WRITE_MULTIPLE_COILS,
    MODBUS_FUNCTION_WRITE_MULTIPLE_REGISTERS};

class ioModbusMaster : public ioModbus, public ioSerial
{
private:
    uint8_t frame[BUFFER_SIZE];
    uint8_t state;
    uint16_t *registers;
    uint16_t errorCount;
    uint8_t validateAnswer(uint8_t);

    void getDiscretes();
    void getRegisters();

public:
    ioModbusMaster() : ioModbus(0), ioSerial(frame, (uint16_t)BUFFER_SIZE){};
    ~ioModbusMaster(){};

    int8_t query(modbus_packet_t);
    int8_t poll();
};

int8_t ioModbusMaster::query(modbus_packet_t telegram)
{
    uint8_t frameSize = 0;

    if (address != 0)
        return -2;
    if (state != COM_IDLE)
        return -1;

    if ((telegram.id == 0) || (telegram.id > 247))
        return -3;

    registers = telegram.registers;
    // telegram header
    frame[0] = telegram.id;
    frame[1] = telegram.function;
    frame[2] = highByte(telegram.startingAddress);
    frame[3] = lowByte(telegram.startingAddress);

    switch (telegram.function)
    {
    case MODBUS_FUNCTION_READ_COILS:
    case MODBUS_FUNCTION_READ_DISCRETE_INPUTS:
    case MODBUS_FUNCTION_READ_HOLDING_REGISTERS:
    case MODBUS_FUNCTION_READ_INPUT_REGISTERS:
        frame[4] = highByte(telegram.noOfRegisters);
        frame[5] = lowByte(telegram.noOfRegisters);
        frameSize = 6;
        break;
    case MODBUS_FUNCTION_WRITE_SINGLE_COIL:
        frame[4] = ((registers[0] > 0) ? 0xff : 0);
        frame[5] = 0;
        frameSize = 6;
        break;
    case MODBUS_FUNCTION_WRITE_SINGLE_REGISTER:
        frame[4] = highByte(registers[0]);
        frame[5] = lowByte(registers[0]);
        frameSize = 6;
        break;
    case MODBUS_FUNCTION_WRITE_MULTIPLE_COILS: // TODO: implement "sending coils"
        uint8_t noOfRegs = telegram.noOfRegisters / 16;
        uint8_t noOfBytes = noOfRegs * 2;
        if ((telegram.noOfRegisters % 16) != 0)
        {
            noOfBytes++;
            noOfRegs++;
        }

        frame[4] = highByte(telegram.noOfRegisters);
        frame[5] = lowByte(telegram.noOfRegisters);
        frame[6] = noOfBytes;
        frameSize = 7;

        for (uint16_t i = 0; i < noOfBytes; i++)
        {
            if (i % 2)
            {
                frame[frameSize] = lowByte(registers[i / 2]);
            }
            else
            {
                frame[frameSize] = highByte(registers[i / 2]);
            }
            frameSize++;
        }
        break;

    case MODBUS_FUNCTION_WRITE_MULTIPLE_REGISTERS:
        frame[4] = highByte(telegram.noOfRegisters);
        frame[5] = lowByte(telegram.noOfRegisters);
        frame[6] = (uint8_t)(telegram.noOfRegisters * 2);
        frameSize = 7;

        for (uint16_t i = 0; i < telegram.noOfRegisters; i++)
        {
            frame[frameSize] = highByte(registers[i]);
            frameSize++;
            frame[frameSize] = lowByte(registers[i]);
            frameSize++;
        }
        break;
    }

    uint16_t crc = calculateCRC(frameSize);
    frame[frameSize] = crc >> 8;
    frameSize++;
    frame[frameSize] = crc & 0x00ff;
    frameSize++;

    sendPacket(frameSize);
    state = COM_WAITING;
    return 0;
}

int8_t ioModbusMaster::poll()
{
    uint8_t frameSize = readPacket();
    if (frameSize < 6) //7 was incorrect for functions 1 and 2 the smallest frame could be 6 bytes long
    {
        state = COM_IDLE;
        return frameSize;
    }

    // validate message: id, CRC, FCT, exception
    uint8_t u8exception = validateAnswer(frameSize);
    if (u8exception != 0)
    {
        state = COM_IDLE;
        return u8exception;
    }

    // process answer
    switch (frame[2])
    {
    case MODBUS_FUNCTION_READ_COILS:
    case MODBUS_FUNCTION_READ_DISCRETE_INPUTS:
        getDiscretes();
        break;
    case MODBUS_FUNCTION_READ_INPUT_REGISTERS:
    case MODBUS_FUNCTION_READ_HOLDING_REGISTERS:
        getRegisters();
        break;
    case MODBUS_FUNCTION_WRITE_SINGLE_COIL:
    case MODBUS_FUNCTION_WRITE_SINGLE_REGISTER:
    case MODBUS_FUNCTION_WRITE_MULTIPLE_COILS:
    case MODBUS_FUNCTION_WRITE_MULTIPLE_REGISTERS:
        // nothing to do
        break;
    default:
        break;
    }
    state = COM_IDLE;
    return frameSize;
}

uint8_t ioModbusMaster::validateAnswer(uint8_t frameSize)
{
    // check message crc vs calculated crc
    uint16_t crc = ((frame[frameSize - 2] << 8) | frame[frameSize - 1]); // combine the crc Low & High bytes
    if (calculateCRC(frameSize - 2) != crc)
    {
        errorCount++;
        return MODBUS_NO_REPLY;
    }

    // check exception
    if ((frame[2] & 0x80) != 0)
    {
        errorCount++;
        return ERR_EXCEPTION;
    }

    // check fct code
    boolean isSupported = false;
    for (uint8_t i = 0; i < sizeof(functionSupported); i++)
    {
        if (functionSupported[i] == frame[2])
        {
            isSupported = true;
            break;
        }
    }
    if (!isSupported)
    {
        errorCount++;
        return MODBUS_EXCEPTION_CODE_ILLEGAL_FUNCTION;
    }

    return 0; // OK, no exception code thrown
}

void ioModbusMaster::getDiscretes()
{
    uint8_t u8byte, i;
    u8byte = 3;
    for (i = 0; i < frame[2]; i++)
    {

        if (i % 2)
        {
            registers[i / 2] = word(frame[i + u8byte], lowByte(registers[i / 2]));
        }
        else
        {

            registers[i / 2] = word(highByte(registers[i / 2]), frame[i + u8byte]);
        }
    }
}

void ioModbusMaster::getRegisters()
{
    uint8_t idx = 3;

    for (uint8_t i = 0; i < frame[2] / 2; i++)
    {
        registers[i] = ((frame[idx] << 8) | frame[idx + 1]);
        idx += 2;
    }
}

#endif