#ifndef _Routine_h
#define _Routine_h

#include "Arduino.h"
#include "UList.h"

typedef void (*OnTick_t)();

typedef struct Task{
  uint32_t interval = 0;
  uint32_t previous = 0;  
  uint32_t current = 0;
  OnTick_t isr;
};

class Routine{
  public:
    Routine();
    void add(uint32_t _interval, OnTick_t _isr);
    void poll();
  private:
    UList<Task> tasks;
};

Routine::Routine(){
  tasks.Clear();
}

void Routine::add(uint32_t _interval, OnTick_t _isr){
  Task task;
  task.interval = _interval;
  task.previous = task.current = millis();
  task.isr = _isr;
  tasks.Add(task);
}

void Routine::poll(){
  for(byte i=0;i<tasks.Count();i++){
    uint32_t currentMillis = millis();
    if(currentMillis - tasks[i].previous >= tasks[i].interval) {
      tasks[i].previous = tasks[i].current = currentMillis;
      tasks[i].isr();
    }
  }
}
#endif