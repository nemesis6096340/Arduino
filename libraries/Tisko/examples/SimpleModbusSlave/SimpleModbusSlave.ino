#include <ioModbusSlave.h>

ioModbusSlave modbusRTU;

uint16_t registers[6]={100,425,30,1727,0,256};

void setup()
{
    modbusRTU.config(Serial, 19200, SERIAL_8E2);
    modbusRTU.setAddress(247);
	modbusRTU.addHoldingRegisters(10, registers, 6);
}

void loop()
{
	modbusRTU.poll();
}
