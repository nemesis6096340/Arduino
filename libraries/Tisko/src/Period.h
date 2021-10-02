#ifndef _Period_h
#define _Period_h

#include "Arduino.h"

#define MIN_LAP   50UL
#define MAX_LAP 5000UL

class Period{
  public:
    void enable(boolean enabled = true);
    void reset();
    uint32_t query();
    uint32_t update();
    uint32_t get();
    uint32_t getRPM() { return(get(  60000UL)); };
    uint32_t getRPH() { return(get(3600000UL)); };
  private:    
    boolean  enabled = false;
    uint32_t duration = 0;
    uint32_t last = 0;
    uint32_t current = 0;
    uint32_t rounder(uint32_t value, byte digits = 1);
    uint32_t get(uint32_t _factor = MAX_LAP);
};
// Public Methods
void Period::enable(boolean enabled){
  this->enabled = enabled;
  reset();
}

void Period::reset(){
  duration = 0;
  last = current = millis();
}

uint32_t Period::query(){
  return (millis()-last);
}

uint32_t Period::update(){
  last = current;
  if(enabled) current = millis();
  duration = current - last;
  return current - last;
}

uint32_t Period::get(){
  return (round(duration));
}

// Private Methods
uint32_t Period::get(uint32_t _factor){
  uint32_t _lap = rounder(duration,2);
  if (_lap >= MIN_LAP && _lap <= MAX_LAP && _lap != 0)
    return _factor/_lap;
  else
    return 0;
}
uint32_t Period::rounder(uint32_t value, byte digits){
  float data = value / pow(10,digits);
  data = round(data);
  value = (uint32_t) data * pow(10,digits);
  return value;
}
#endif