#ifndef _Modbus_h
#define _Modbus_h

#include "Arduino.h"

#define BUFFER_SIZE 64

class Modbus {
  public :
    Modbus(byte id) {
      slaveID = id;
    };
    
    void setAddress(byte id){
      slaveID = id;
    };

    void setRegisters(word *Registers, word sizeRegister) {
      this->Registers =  Registers;
      this->sizeRegister = sizeRegister;
    };

    word receivePDU(byte* inputFrame, word sizeInput, byte* outputFrame) {
      byte id = inputFrame[0];
      word sizeOutput=0;
      if (id == slaveID) {
        byte function = inputFrame[1];
        unsigned int startingAddress = ((inputFrame[2] << 8) | inputFrame[3]); // combine the starting address bytes
        unsigned int no_of_registers = ((inputFrame[4] << 8) | inputFrame[5]); // combine the number of register bytes
        unsigned int maxData = startingAddress + no_of_registers;
        byte index;
        byte address;

        switch (function) {
          case 3:
            // check exception 2 ILLEGAL DATA ADDRESS
            if (startingAddress < sizeRegister) {
              // check exception 3 ILLEGAL DATA VALUE
              if (maxData <= sizeRegister) {
                unsigned char noOfBytes = no_of_registers * 2;
                unsigned char responseFrameSize = 5 + noOfBytes; // ID, function, noOfBytes, (dataLo + dataHi) * number of registers, crcLo, crcHi
                outputFrame[0] = slaveID;
                outputFrame[1] = function;
                outputFrame[2] = noOfBytes;
                address = 3; // PDU starts at the 4th byte
                unsigned int temp;

                for (index = startingAddress; index < maxData; index++) {
                  temp = Registers[index];
                  outputFrame[address] = temp >> 8; // split the register into 2 bytes
                  address++;
                  outputFrame[address] = temp & 0xFF;
                  address++;
                }
                sizeOutput = address;
              }
              else{
                sizeOutput = exceptionResponse(3,3,outputFrame); // exception 3 ILLEGAL DATA VALUE
              }
            }
            else{
              sizeOutput = exceptionResponse(2,3,outputFrame); // exception 2 ILLEGAL DATA ADDRESS
            }
            break;
          case 6:
            // check exception 2 ILLEGAL DATA ADDRESS
            if (startingAddress < sizeRegister){
              unsigned int startingAddress = ((inputFrame[2] << 8) | inputFrame[3]);
              unsigned int regStatus = ((inputFrame[4] << 8) | inputFrame[5]);
              Registers[startingAddress] = regStatus;
              memcpy(outputFrame, inputFrame, 6);
              sizeOutput = 6;
            }
            else{
              sizeOutput = exceptionResponse(2,6,outputFrame); // exception 2 ILLEGAL DATA ADDRESS
            }
            break;
          case 16:
            // check if the recieved number of bytes matches the calculated bytes minus the request bytes
            // id + function + (2 * address bytes) + (2 * no of register bytes) + byte count + (2 * CRC bytes) = 9 bytes
            if (inputFrame[6] == (sizeInput - 9)){
              // check exception 2 ILLEGAL DATA ADDRESS
              if (startingAddress < sizeRegister){
                // check exception 3 ILLEGAL DATA VALUE
                if (maxData <= sizeRegister){
                  address = 6; // start at the 8th byte in the inputFrame
                  for (byte index = startingAddress; index < maxData; index++)
                  {
                    Registers[index] = ((inputFrame[address] << 8) | inputFrame[address + 1]);
                    address += 2;
                  }
                  memcpy(outputFrame, inputFrame, 6);
                  sizeOutput = 6;
                }
                else{
                  sizeOutput = exceptionResponse(3,16,outputFrame); // exception 3 ILLEGAL DATA VALUE
                }
              }
              else{
                sizeOutput = exceptionResponse(2,16,outputFrame); // exception 2 ILLEGAL DATA ADDRESS
              }
            }
            else
              sizeOutput = 0;
            break;
          default:
            sizeOutput = exceptionResponse(1,16,outputFrame); // exception 1 ILLEGAL FUNCTION
        }
      } // incorrect id
      return sizeOutput;
    };
    
    byte getAddress(){
      return slaveID;
    };
  private:
    byte slaveID;
    word *Registers;
    word sizeRegister;
    word errorCount = 0;

    word exceptionResponse(byte exception, byte function, byte* outputFrame) {
      errorCount++; // each call to exceptionResponse() will increment the errorCount
      outputFrame[0] = slaveID;
      outputFrame[1] = (function | 0x80); // set the MSB bit high, informs the master of an exception
      outputFrame[2] = exception;
      return 3;
    };
};
#endif