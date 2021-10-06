#ifndef _IO_MODBUS_h
#define _IO_MODBUS_h

#include "Arduino.h"

// Address
enum
{
  MODBUS_ADDRESS_DISCRETE_COILS = 00001,
  MODBUS_ADDRESS_DISCRETE_INPUTS = 10001,
  MODBUS_ADDRESS_INPUT_REGISTERS = 30001,
  MODBUS_ADDRESS_HOLDING_REGISTERS = 40001,
};

//Function Codes
enum
{
  MODBUS_FUNCTION_READ_COILS = 0x01,               // Read Coils (Output) Status 0xxxx
  MODBUS_FUNCTION_READ_DISCRETE_INPUTS = 0x02,     // Read Input Status (Discrete Inputs) 1xxxx
  MODBUS_FUNCTION_READ_HOLDING_REGISTERS = 0x03,   // Read Holding Registers 4xxxx
  MODBUS_FUNCTION_READ_INPUT_REGISTERS = 0x04,     // Read Input Registers 3xxxx
  MODBUS_FUNCTION_WRITE_SINGLE_COIL = 0x05,        // Write Single Coil (Output) 0xxxx
  MODBUS_FUNCTION_WRITE_SINGLE_REGISTER = 0x06,    // Preset Single Register 4xxxx
  MODBUS_FUNCTION_WRITE_MULTIPLE_COILS = 0x0F,     // Write Multiple Coils (Outputs) 0xxxx
  MODBUS_FUNCTION_WRITE_MULTIPLE_REGISTERS = 0x10, // Write block of contiguous registers 4xxxx
};

//Exception Codes
enum
{
  MODBUS_EXCEPTION_CODE_ILLEGAL_FUNCTION = 0x01, // Function Code not Supported
  MODBUS_EXCEPTION_CODE_ILLEGAL_ADDRESS = 0x02,  // Output Address not exists
  MODBUS_EXCEPTION_CODE_ILLEGAL_VALUE = 0x03,    // Output Value not in Range
  MODBUS_EXCEPTION_CODE_SLAVE_FAILURE = 0x04,    // Slave Deive Fails to process request
};

typedef struct register_t
{
  uint16_t address;
  uint16_t *registers;
  uint16_t size;
  struct register_t *next;
} register_t;

//const int sizeRegs = sizeof(register_t);

typedef struct discreet_t
{
  uint16_t address;
  uint8_t *discrets;
  uint16_t size;
  struct discreet_t *next;
} discreet_t;

//const int sizeRegs = sizeof(register_t);

class ioModbus
{
protected:
  uint8_t address;
  uint8_t *frame;

  uint16_t errorCount = 0;

private:
  register_t *_regs_head;
  register_t *_regs_last;

  discreet_t *_disc_head;
  discreet_t *_disc_last;
  //uint8_t function;

  void addRegisters(uint16_t, uint16_t *, uint16_t);
  void addDiscrets(uint16_t, uint8_t *, uint8_t);

  register_t *searchRegister(uint16_t);
  discreet_t *searchDiscreet(uint16_t);

  uint16_t exceptionResponse(uint8_t function, uint8_t exception)
  {
    errorCount++; // each call to exceptionResponse() will increment the errorCount
    frame[0] = address;
    frame[1] = (function | 0x80); // set the MSB bit high, informs the master of an exception
    frame[2] = exception;
    return 3;
  };

public:
  ioModbus(uint8_t address)
  {
    this->address = address;
  };

  void setAddress(uint8_t address)
  {
    this->address = address;
  };

  void addDiscretsCoil(uint16_t, uint8_t *, uint8_t);
  void addDiscretsInput(uint16_t, uint8_t *, uint8_t);

  void addHoldingRegisters(uint16_t, uint16_t *, uint16_t);
  void addInputRegisters(uint16_t, uint16_t *, uint16_t);

  uint16_t readDiscrets(uint16_t, uint16_t, uint16_t);
  uint16_t writeSingleCoil(uint16_t);
  uint16_t writeMultipleCoils(uint16_t, uint16_t);

  uint16_t readRegisters(uint16_t, uint16_t, uint16_t);
  uint16_t writeSingleRegister(uint16_t);
  uint16_t writeMultipleRegisters(uint16_t, uint16_t);

  uint16_t receivePDU(uint8_t *, uint16_t);

  void printBuffer(uint8_t *, uint8_t);
  void printRegister(uint16_t *, uint16_t);
};

void ioModbus::printRegister(uint16_t *registers, uint16_t size)
{
#ifdef _DEBUG_IO_MODBUS
  Serial.print("Register: ");
  for (uint16_t i = 0; i < size; i++)
  {
    char data[7];
    sprintf(data, "[%04X]", registers[i]);
    Serial.print(data);
  }
  Serial.print("\tSize: ");
  Serial.println(size);
#endif
};

void ioModbus::printBuffer(uint8_t *buffer, uint8_t size)
{
#ifdef _DEBUG_IO_MODBUS
  Serial.print("Buffer: ");
  for (uint8_t i = 0; i < size; i++)
  {
    char data[5];
    sprintf(data, "[%02X]", buffer[i]);
    Serial.print(data);
  }
  Serial.print("\tSize: ");
  Serial.println(size);
#endif
};

uint16_t ioModbus::receivePDU(uint8_t *frameInput, uint16_t sizeInput)
{
  uint8_t id = frame[0];
  this->frame = frameInput;

  uint16_t sizeOutput = 0;
  if (id == address)
  {
    uint8_t function = frame[1];
    uint16_t startingAddress = ((frame[2] << 8) | frame[3]); // combine the starting address bytes
    uint16_t no_of_registers = ((frame[4] << 8) | frame[5]); // combine the number of register bytes
    switch (function)
    {
    case MODBUS_FUNCTION_READ_HOLDING_REGISTERS:
      sizeOutput = readRegisters(startingAddress, no_of_registers, MODBUS_ADDRESS_HOLDING_REGISTERS);
      break;
    case MODBUS_FUNCTION_READ_INPUT_REGISTERS:
      sizeOutput = readRegisters(startingAddress, no_of_registers, MODBUS_ADDRESS_INPUT_REGISTERS);
      break;
    case MODBUS_FUNCTION_WRITE_SINGLE_REGISTER:
      sizeOutput = writeSingleRegister(startingAddress);
      break;
    case MODBUS_FUNCTION_WRITE_MULTIPLE_REGISTERS:
      if (frame[6] == (sizeInput - 9))
        sizeOutput = writeMultipleRegisters(startingAddress, no_of_registers);
      else
        sizeOutput = 0;
      break;
#ifndef USE_HOLDING_REGISTERS_ONLY
    case MODBUS_FUNCTION_READ_COILS:
      sizeOutput = readDiscrets(startingAddress, no_of_registers, MODBUS_ADDRESS_DISCRETE_COILS);
      break;

    case MODBUS_FUNCTION_READ_DISCRETE_INPUTS:
      sizeOutput = readDiscrets(startingAddress, no_of_registers, MODBUS_ADDRESS_DISCRETE_INPUTS);
      break;

    case MODBUS_FUNCTION_WRITE_SINGLE_COIL:
      sizeOutput = writeSingleCoil(startingAddress);
      break;

    case MODBUS_FUNCTION_WRITE_MULTIPLE_COILS:
      sizeOutput = writeMultipleCoils(startingAddress, no_of_registers);
      break;
#endif
    default:
      sizeOutput = exceptionResponse(function, MODBUS_EXCEPTION_CODE_ILLEGAL_FUNCTION); // exception 1 ILLEGAL FUNCTION
    }
  } // incorrect address
  return sizeOutput;
};

void ioModbus::addRegisters(uint16_t address, uint16_t *registers, uint16_t size)
{
  register_t *newreg;

  newreg = (register_t *)malloc(sizeof(register_t));
  newreg->address = address;
  newreg->registers = registers;
  newreg->size = size;
  newreg->next = 0;

  if (_regs_head == 0)
  {
    _regs_head = newreg;
    _regs_last = _regs_head;
  }
  else
  {
    //Assign the last register's next pointer to newreg.
    _regs_last->next = newreg;
    //then make temp the last register in the list.
    _regs_last = newreg;
  }
}

void ioModbus::addHoldingRegisters(uint16_t offset, uint16_t *holdingRegisters, uint16_t sizeHoldingRegister)
{
  this->addRegisters(offset + MODBUS_ADDRESS_HOLDING_REGISTERS, holdingRegisters, sizeHoldingRegister);
};

void ioModbus::addInputRegisters(uint16_t offset, uint16_t *holdingRegisters, uint16_t sizeHoldingRegister)
{
  this->addRegisters(offset + MODBUS_ADDRESS_INPUT_REGISTERS, holdingRegisters, sizeHoldingRegister);
};

uint16_t ioModbus::readDiscrets(uint16_t startingAddress, uint16_t no_of_registers, uint16_t functionAddress)
{
  discreet_t *searchDisc;
  searchDisc = this->searchDiscreet(startingAddress + functionAddress);

  uint16_t maxData = startingAddress + no_of_registers; //6

  if (searchDisc != 0)
  {
    //Serial.println(startingAddress);
    //Serial.print("Adress: ");
    //Serial.println(searchDisc->address);
    uint8_t function = frame[1];
    uint16_t offset = searchDisc->address - functionAddress;
    //Serial.print("Offset: ");
    //Serial.println(offset);

    // check exception 2 ILLEGAL DATA ADDRESS
    //Serial.print(startingAddress + functionAddress + no_of_registers); Serial.print("<="); Serial.println(searchDisc->address + searchDisc->size * 8);
    if (startingAddress + functionAddress + no_of_registers <= searchDisc->address + searchDisc->size * 8)
    {
      // check exception 3 ILLEGAL DATA VALUE
      if (maxData + functionAddress <= searchDisc->address + searchDisc->size * 8)
      {
        uint8_t noOfBytes = no_of_registers / 8;
        if (no_of_registers % 8 > 0)
          noOfBytes++;
        //Serial.print("NoBytes: ");
        //Serial.println(noOfBytes);

        frame[0] = address;
        frame[1] = function;
        frame[2] = noOfBytes;

        uint8_t temp = 0;

        uint16_t diff = startingAddress - offset;
        for (uint16_t index = startingAddress; index < maxData; index++)
        {

          uint16_t pos = index - offset;
          uint8_t i = (pos - diff) / 8;
          uint8_t j = (pos - diff) % 8;
          if (j == 0)
            temp = 0;

          /*Serial.print(pos);
          Serial.print(';');
          Serial.print(i);
          Serial.print(':');
          Serial.print(j);
          Serial.print('=');
        
          Serial.print(bitRead(searchDisc->discrets[pos / 8], pos % 8));
          Serial.print('\t');
          */
          bitWrite(temp, j, bitRead(searchDisc->discrets[pos / 8], pos % 8));
          frame[3 + i] = temp;
          /*
          boolean temp = bitRead(searchDisc->discrets[i], bitn);
          if (temp)
            bitSet(frame[3 + i], bitn);
          else
            bitClear(frame[3 + i], bitn);*/
        }
        //Serial.println();
        return noOfBytes + 3;
      }
      else
        return exceptionResponse(function, MODBUS_EXCEPTION_CODE_ILLEGAL_VALUE); // exception 3 ILLEGAL DATA VALUE
    }
    else
      return exceptionResponse(function, MODBUS_EXCEPTION_CODE_ILLEGAL_ADDRESS); // exception 2 ILLEGAL DATA ADDRESS*/
  }

  else
    return exceptionResponse(MODBUS_FUNCTION_READ_HOLDING_REGISTERS, MODBUS_EXCEPTION_CODE_ILLEGAL_ADDRESS);
}

uint16_t ioModbus::writeSingleCoil(uint16_t startingAddress)
{
  discreet_t *searchDisc;
  uint8_t functionAddress = MODBUS_ADDRESS_DISCRETE_COILS;
  uint8_t function = frame[1];

  searchDisc = this->searchDiscreet(startingAddress + functionAddress);
  if (searchDisc != 0)
  {
    uint16_t offset = searchDisc->address - functionAddress;
    uint16_t maxData = startingAddress + 1;
    //check exception 2 ILLEGAL DATA ADDRESS
    if (startingAddress + functionAddress <= searchDisc->address + searchDisc->size * 8)
    {
      // check exception 3 ILLEGAL DATA VALUE
      if (maxData + functionAddress <= searchDisc->address + searchDisc->size * 8)
      {
        // [07][05][01][F8][00][00]
        // [07][05][01][FC][FF][00]
        uint16_t status = ((frame[4] << 8) | frame[5]);
        if (status != 0xFF00 && status != 0x0000)
          return exceptionResponse(function, MODBUS_EXCEPTION_CODE_ILLEGAL_VALUE);
        else
        {
          uint16_t pos = startingAddress - offset;
          bitWrite(searchDisc->discrets[pos / 8], pos % 8, (bool)status);
          return 6;
        }
      }
      else
        return exceptionResponse(function, MODBUS_EXCEPTION_CODE_ILLEGAL_VALUE); // exception 3 ILLEGAL DATA VALUE
    }
    else
      return exceptionResponse(function, MODBUS_EXCEPTION_CODE_ILLEGAL_ADDRESS); // exception 2 ILLEGAL DATA ADDRESS*/
  }
  else
    return exceptionResponse(function, MODBUS_EXCEPTION_CODE_ILLEGAL_ADDRESS);
}
uint16_t ioModbus::writeMultipleCoils(uint16_t startingAddress, uint16_t no_of_discrets)
{
  //[07][0F][01][F8][00][01][01][00]	Size: 8
  //[07][0F][01][F4][00][08][01][55]
  discreet_t *searchDisc;
  searchDisc = this->searchDiscreet(startingAddress + MODBUS_ADDRESS_DISCRETE_COILS);

  if (searchDisc != 0)
  {
    uint8_t function = frame[1];

    uint16_t offset = searchDisc->address - MODBUS_ADDRESS_DISCRETE_COILS;
    uint16_t maxData = startingAddress + no_of_discrets;

    // check exception 2 ILLEGAL DATA ADDRESS
    //Serial.print(startingAddress + MODBUS_ADDRESS_DISCRETE_COILS + no_of_discrets); Serial.print("<="); Serial.println(searchDisc->address + searchDisc->size * 8);
    if (startingAddress + MODBUS_ADDRESS_DISCRETE_COILS + no_of_discrets <= searchDisc->address + searchDisc->size * 8)
    {
      // check exception 3 ILLEGAL DATA VALUE
      if (maxData + MODBUS_ADDRESS_DISCRETE_COILS <= searchDisc->address + searchDisc->size * 8)
      {
        uint8_t noOfBytes = frame[6];
        for (uint16_t idx = 0; idx < noOfBytes; idx++)
        {
          uint8_t temp = frame[7 + idx];
          uint16_t diff = startingAddress - offset;
          for (uint16_t index = startingAddress; index < maxData; index++)
          {
            uint16_t pos = index - offset;
            uint8_t i = (pos - diff) / 8;
            uint8_t j = (pos - diff) % 8;
            bitWrite(searchDisc->discrets[pos / 8], pos % 8, bitRead(temp, j));
          }
        }
        return 6;
      }
      else
        return exceptionResponse(function, MODBUS_EXCEPTION_CODE_ILLEGAL_VALUE); // exception 3 ILLEGAL DATA VALUE
    }
    else
      return exceptionResponse(function, MODBUS_EXCEPTION_CODE_ILLEGAL_ADDRESS); // exception 2 ILLEGAL DATA ADDRESS*/
  }
  else
    return exceptionResponse(MODBUS_FUNCTION_READ_HOLDING_REGISTERS, MODBUS_EXCEPTION_CODE_ILLEGAL_ADDRESS);
}

uint16_t ioModbus::readRegisters(uint16_t startingAddress, uint16_t no_of_registers, uint16_t functionAddress)
{
  register_t *searchReg;
  searchReg = this->searchRegister(startingAddress + functionAddress);

  uint16_t maxData = startingAddress + no_of_registers;

  if (searchReg != 0)
  {
    uint8_t function = frame[1];
    uint16_t offset = searchReg->address - functionAddress;

    // check exception 2 ILLEGAL DATA ADDRESS
    if (startingAddress < searchReg->address + searchReg->size)
    {
      // check exception 3 ILLEGAL DATA VALUE
      if (maxData <= searchReg->address + searchReg->size)
      {
        uint8_t noOfBytes = no_of_registers * 2;
        uint8_t responseFrameSize = 5 + noOfBytes; // Address, function, noOfBytes, (dataLo + dataHi) * number of registers, crcLo, crcHi
        frame[0] = address;
        frame[1] = function;
        frame[2] = noOfBytes;
        uint8_t address = 3; // PDU starts at the 4th byte
        //unsigned int temp;

        for (uint16_t index = startingAddress; index < maxData; index++)
        {
          uint16_t pos = index - offset;
          uint16_t temp = searchReg->registers[pos];
          frame[address] = temp >> 8; // split the register into 2 bytes
          address++;
          frame[address] = temp & 0xFF;
          address++;
        }
        return address;
      }
      else
        return exceptionResponse(function, MODBUS_EXCEPTION_CODE_ILLEGAL_VALUE); // exception 3 ILLEGAL DATA VALUE
    }
    else
      return exceptionResponse(function, MODBUS_EXCEPTION_CODE_ILLEGAL_ADDRESS); // exception 2 ILLEGAL DATA ADDRESS
  }
  else
    return exceptionResponse(MODBUS_FUNCTION_READ_HOLDING_REGISTERS, MODBUS_EXCEPTION_CODE_ILLEGAL_ADDRESS);
}

uint16_t ioModbus::writeSingleRegister(uint16_t startingAddress)
{
  register_t *searchReg;
  searchReg = this->searchRegister(startingAddress + MODBUS_ADDRESS_HOLDING_REGISTERS);

  if (searchReg != 0)
  {
    uint16_t offset = searchReg->address - MODBUS_ADDRESS_HOLDING_REGISTERS;
    //check exception 2 ILLEGAL DATA ADDRESS
    if (startingAddress < offset + searchReg->size)
    {
      //uint16_t startingAddress = ((frame[2] << 8) | frame[3]);
      uint16_t regStatus = ((frame[4] << 8) | frame[5]);
      uint16_t pos = startingAddress - offset;
      searchReg->registers[pos] = regStatus;
      return 6;
    }
    else
      return exceptionResponse(MODBUS_FUNCTION_WRITE_SINGLE_REGISTER, MODBUS_EXCEPTION_CODE_ILLEGAL_ADDRESS); // exception 2 ILLEGAL DATA ADDRESS
  }
}

uint16_t ioModbus::writeMultipleRegisters(uint16_t startingAddress, uint16_t no_of_registers)
{
  register_t *searchReg;
  searchReg = this->searchRegister(startingAddress + MODBUS_ADDRESS_HOLDING_REGISTERS);

  if (searchReg != 0)
  {
    uint16_t offset = searchReg->address - MODBUS_ADDRESS_HOLDING_REGISTERS;

    uint16_t maxData = startingAddress + no_of_registers;
    // check exception 2 ILLEGAL DATA ADDRESS
    if (startingAddress < searchReg->address + searchReg->size)
    {
      // check exception 3 ILLEGAL DATA VALUE
      if (maxData <= searchReg->address + searchReg->size)
      {
        uint8_t address = 7; // start at the 8th byte in the frame
        for (uint16_t index = startingAddress; index < maxData; index++)
        {
          uint16_t pos = index - offset;
          searchReg->registers[pos] = ((frame[address] << 8) | frame[address + 1]);
          address += 2;
        }
        return 6;
      }
      else
        return exceptionResponse(MODBUS_FUNCTION_WRITE_MULTIPLE_REGISTERS, MODBUS_EXCEPTION_CODE_ILLEGAL_VALUE); // exception 3 ILLEGAL DATA VALUE
    }
    else
      return exceptionResponse(MODBUS_FUNCTION_WRITE_MULTIPLE_REGISTERS, MODBUS_EXCEPTION_CODE_ILLEGAL_ADDRESS); // exception 2 ILLEGAL DATA ADDRESS
  }
  else
    return exceptionResponse(MODBUS_FUNCTION_WRITE_MULTIPLE_REGISTERS, MODBUS_EXCEPTION_CODE_ILLEGAL_ADDRESS);
}

register_t *ioModbus::searchRegister(uint16_t address)
{
  register_t *reg = _regs_head;
  //if there is no register configured, bail
  if (reg == 0)
    return (0);
  //scan through the linked list until the end of the list or the register is found.
  //return the pointer.
  do
  {
    //if (reg->address == address)
    //  return (reg);
    if (address >= reg->address && address <= (reg->address + reg->size))
      return (reg);
    reg = reg->next;
  } while (reg);
  return (0);
}

void ioModbus::addDiscrets(uint16_t address, uint8_t *discrets, uint8_t size)
{
  discreet_t *newdisc;

  newdisc = (discreet_t *)malloc(sizeof(discreet_t));
  newdisc->address = address;
  newdisc->discrets = discrets;
  newdisc->size = size;
  newdisc->next = 0;

  if (_disc_head == 0)
  {
    _disc_head = newdisc;
    _disc_last = _disc_head;
  }
  else
  {
    //Assign the last register's next pointer to newreg.
    _disc_last->next = newdisc;
    //then make temp the last register in the list.
    _disc_last = newdisc;
  }
}

void ioModbus::addDiscretsCoil(uint16_t offset, uint8_t *discrets, uint8_t size)
{
  addDiscrets(offset + MODBUS_ADDRESS_DISCRETE_COILS, discrets, size);
}

void ioModbus::addDiscretsInput(uint16_t offset, uint8_t *discrets, uint8_t size)
{
  addDiscrets(offset + MODBUS_ADDRESS_DISCRETE_INPUTS, discrets, size);
}

discreet_t *ioModbus::searchDiscreet(uint16_t address)
{
  discreet_t *disc = _disc_head;
  //if there is no register configured, bail
  if (disc == 0)
    return (0);
  //scan through the linked list until the end of the list or the register is found.
  //return the pointer.
  do
  {
    //if (reg->address == address)
    //  return (reg);
    if (address >= disc->address && address <= disc->address + disc->size * 8)
      return (disc);
    disc = disc->next;
  } while (disc);
  return (0);
}

#endif