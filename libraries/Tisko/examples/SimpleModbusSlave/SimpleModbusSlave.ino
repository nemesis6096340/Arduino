#include <ioModbusSlave.h>

ioModbusSlave modbusRTU;

uint8_t discretInput[1] = {0b00000011};
uint8_t discretOutput[1] = {0b00000000};

const uint8_t pinMask_DIN[] = {2, 3};
const uint8_t pinMask_DOUT[] = {7, 8, A2, A3};
const uint8_t pinMask_AIN[] = {A0, A1};
const uint8_t pinMask_AOUT[] = {5, 6};

const uint8_t NUM_DISCRETE_INPUT = sizeof(pinMask_DIN);
const uint8_t NUM_DISCRETE_COILS = sizeof(pinMask_DOUT);
const uint8_t NUM_INPUT_REGISTERS = sizeof(pinMask_AIN);
const uint8_t NUM_HOLDING_REGISTERS = sizeof(pinMask_AOUT);

uint16_t inputRegisters[NUM_INPUT_REGISTERS];
uint16_t holdingRegisters[NUM_HOLDING_REGISTERS];

void setup()
{
    //Serial.begin(19200, SERIAL_8E2);
    configurePins();
    
    modbusRTU.config(Serial, 19200, SERIAL_8E2);
    modbusRTU.setAddress(7);

    modbusRTU.addDiscretsInput(100, discretInput, 1);
    modbusRTU.addDiscretsCoil(100, discretOutput, 1);
    modbusRTU.addHoldingRegisters(100, holdingRegisters, NUM_HOLDING_REGISTERS);
    modbusRTU.addInputRegisters(100, inputRegisters, NUM_INPUT_REGISTERS);
}

void loop()
{
	modbusRTU.poll();
    update();
}

void configurePins()
{
    for (int i = 0; i < NUM_DISCRETE_INPUT; i++)
    {
        pinMode(pinMask_DIN[i], INPUT_PULLUP);
    }

    for (int i = 0; i < NUM_DISCRETE_COILS; i++)
    {
        pinMode(pinMask_DOUT[i], OUTPUT);
    }

    for (int i = 0; i < NUM_INPUT_REGISTERS; i++)
    {
        pinMode(pinMask_AIN[i], INPUT);
    }

    for (int i = 0; i < NUM_HOLDING_REGISTERS; i++)
    {
        pinMode(pinMask_AOUT[i], OUTPUT);
    }
}

void update()
{
    // DISCRETE COILS
    for (int i = 0; i < NUM_DISCRETE_COILS; ++i)
    {
        digitalWrite(
            pinMask_DOUT[i],
            !bitRead(discretOutput[i / 8], i % 8));
    }
    // DISCRETE INPUT
    for (int i = 0; i < NUM_DISCRETE_INPUT; ++i)
    {
        uint8_t idx = i / 8;
        bitWrite(
            discretInput[i / 8],
            i % 8,
            digitalRead(pinMask_DIN[i]));
    }
    // INPUT REGISTER
    for (int i = 0; i < NUM_INPUT_REGISTERS; ++i)
    {
        inputRegisters[i] = (analogRead(pinMask_AIN[i]) * 64);
    }
    // HOLDING REGISTER
    for (int i = 0; i < NUM_HOLDING_REGISTERS; ++i)
    {
        analogWrite(pinMask_AOUT[i], (holdingRegisters[i] / 256));
    }
}
