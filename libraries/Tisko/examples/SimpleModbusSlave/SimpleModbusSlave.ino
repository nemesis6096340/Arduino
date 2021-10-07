#include <ioModbusSlave.h>

ioModbusSlave modbusRTU(Serial1);

uint16_t registers[6]={100,425,30,1727,0,256};

void setup()
{
    Serial.begin(19200, SERIAL_8E2);
    modbusRTU.begin(19200, SERIAL_8E2);
    modbusRTU.setAddress(1);
	modbusRTU.addHoldingRegisters(10, registers, 6);
    Serial.println("run...");
}

void loop()
{
	modbusRTU.poll();
}
