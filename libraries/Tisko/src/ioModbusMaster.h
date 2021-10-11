#include "Arduino.h"
#include "ioModbus.h"
#include "ioSerial.h"

#ifndef _IO_MODBUS_MASTER_H
#define _IO_MODBUS_MASTER_H

class ioModbusMaster : public ioModbus, public ioSerial
{
private:
    /* data */
public:
    ioModbusMaster(/* args */);
    ~ioModbusMaster();
};

ioModbusMaster::ioModbusMaster(/* args */)
{
}

ioModbusMaster::~ioModbusMaster()
{
}


#endif