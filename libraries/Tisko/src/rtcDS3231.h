#ifndef _RTC_DS3231_h
#define _RTC_DS3231_h

#include <Arduino.h>
#include "ioWire.h"

#define REG_CON 0x0E
#define REG_STATUS 0x0F
#define REG_AGING 0x10
#define REG_TEMPM 0x11
#define REG_TEMPL 0x12

#define SQW_RATE_1 0
#define SQW_RATE_1K 1
#define SQW_RATE_4K 2
#define SQW_RATE_8K 3

#define OUTPUT_SQW 0
#define OUTPUT_INT 1

#define SEC_1970_TO_2000 946684800UL
static const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

class Time
{
public:
  uint8_t hour = 0;
  uint8_t min = 0;
  uint8_t sec = 0;
  uint8_t day = 1;
  uint8_t mon = 1;
  uint16_t year = 2021;
  uint8_t dow = 5;

  Time(){};
};

class rtcDS3231
{
public:
  rtcDS3231(){};

  void begin()
  {
    wire.begin(DS3231_register, 0x68);
  };

  Time getTime()
  {
    Time t;
    wire.readRegisters(0, 7);
    t.sec = _decode(DS3231_register[0]);
    t.min = _decode(DS3231_register[1]);
    t.hour = _decodeH(DS3231_register[2]);
    t.dow = DS3231_register[3];
    t.day = _decode(DS3231_register[4]);
    t.mon = _decode(DS3231_register[5]);
    t.year = _decodeY(DS3231_register[6]) + 2000;
    return t;
  };

  void setTime(Time t)
  {
    DS3231_register[0] = _encode(t.sec);                 // set seconds
    DS3231_register[1] = _encode(t.min);                 // set minutes
    DS3231_register[2] = _encode(t.hour);                // set hours
    DS3231_register[3] = t.dow;                          // set day of week (1=Sunday, 7=Saturday)
    DS3231_register[4] = _encode(t.day);                 // set date (1 to 31)
    DS3231_register[5] = _encode(t.mon);                 // set month
    DS3231_register[6] = _encode((byte)(t.year - 2000)); // set year (0 to 99)

    wire.writeRegisters(0, 7);
  }

  unsigned long getUnixTime(Time t)
  {
    uint16_t dc;
    dc = t.day;
    for (uint8_t i = 0; i < (t.mon - 1); i++)
      dc += daysInMonth[i];
    if ((t.mon > 2) && (((t.year - 2000) % 4) == 0))
      ++dc;
    dc = dc + (365 * (t.year - 2000)) + (((t.year - 2000) + 3) / 4) - 1;
    return ((((((dc * 24L) + t.hour) * 60) + t.min) * 60) + t.sec) + SEC_1970_TO_2000;
  };

  void setUnixTime(unsigned long unixTime)
  {
    Time t;

    t.sec = (unixTime % 60);
    t.min = (unixTime % 3600L) / 60;
    t.hour = (unixTime % 86400L) / 3600;
    unixTime /= 86400L;

    //Unix time starts in 1970 on a Thursday
    t.year = 1970;
    t.dow = 4;

    while (1)
    {
      bool leapYear = (t.year % 4 == 0 && (t.year % 100 != 0 || t.year % 400 == 0));
      uint16_t daysInYear = leapYear ? 366 : 365;
      if (unixTime >= daysInYear)
      {
        t.dow += leapYear ? 2 : 1;
        unixTime -= daysInYear;
        if (t.dow >= 7)
          t.dow -= 7;
        ++t.year;
      }
      else
      {
        t.dow = (unixTime + t.dow) % 7;
        //static const uint8_t daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        for (t.mon = 0; t.mon < 12; ++t.mon)
        {
          uint8_t dim = daysInMonth[t.mon];
          // add a day to feburary if this is a leap year
          if (t.mon == 1 && leapYear)
            ++dim;
          if (unixTime >= dim)
            unixTime -= dim;
          else
            break;
        }
        t.day = unixTime + 1;
        t.mon++;
        t.dow = t.dow == 0 ? 7 : t.dow;
        break;
      }
    }
    setTime(t);
  };

  float getTemperature()
  {
    uint8_t _msb = wire.readRegister(REG_TEMPM);
    uint8_t _lsb = wire.readRegister(REG_TEMPL);
    return (float)_msb + ((_lsb >> 6) * 0.25f);
  };
  /*
  void enable32KHz(bool enable)
  {
    uint8_t _reg = readRegister(REG_STATUS);
    _reg &= ~(1 << 3);
    _reg |= (enable << 3);
    writeRegister(REG_STATUS, _reg);
  };

  void setOutput(byte enable)
  {
    uint8_t _reg = readRegister(REG_CON);
    _reg &= ~(1 << 2);
    _reg |= (enable << 2);
    writeRegister(REG_CON, _reg);
  };

  void setSQWRate(int rate)
  {
    uint8_t _reg = readRegister(REG_CON);
    _reg &= ~(3 << 3);
    _reg |= (rate << 3);
    writeRegister(REG_CON, _reg);
  };
  */

  byte DS3231_register[0x13]; // create array for the register contents
private:
  ioWire wire;

  byte BCD2DEC(byte val)
  {
    return ((val / 10 * 16) + (val % 10));
  };

  byte DEC2BCD(byte val)
  {
    return ((val / 16 * 10) + (val % 16));
  };

  uint8_t _decode(uint8_t value)
  {
    uint8_t decoded = value & 127;
    decoded = (decoded & 15) + 10 * ((decoded & (15 << 4)) >> 4);
    return decoded;
  };

  uint8_t _decodeH(uint8_t value)
  {
    if (value & 128)
      value = (value & 15) + (12 * ((value & 32) >> 5));
    else
      value = (value & 15) + (10 * ((value & 48) >> 4));
    return value;
  };

  uint8_t _decodeY(uint8_t value)
  {
    uint8_t decoded = (value & 15) + 10 * ((value & (15 << 4)) >> 4);
    return decoded;
  };

  uint8_t _encode(uint8_t value)
  {
    uint8_t encoded = ((value / 10) << 4) + (value % 10);
    return encoded;
  };
};

#endif
