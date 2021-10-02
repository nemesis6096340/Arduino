#include <Arduino.h>
#include "Period.h"

#ifndef COUNTER_H
#define COUNTER_H

uint8_t pinSensor[] = {2, 3};

typedef void (*OnTick_t)();

typedef struct debounce_t
{
    uint8_t buttonState = HIGH;
    uint8_t lastButtonState = LOW;
    uint32_t lastDebounceTime = 0;
};

typedef struct total_t
{
    uint32_t TOTAL;
    uint32_t LAP;
};

typedef struct counter_t
{
    uint16_t STATUS;
    uint16_t THRESHOLD;
    uint16_t TIMEOUT;
    total_t total[sizeof(pinSensor)];
};

const int SIZE_BUFFER_COUNTER = sizeof(counter_t) / sizeof(word);

typedef union production_t
{
    counter_t counter;
    word buffer[SIZE_BUFFER_COUNTER] = {0};
};

class Counter
{
private:
    OnTick_t counting;
    OnTick_t stopped;

    debounce_t debounceDIN[sizeof(pinSensor)];
    uint32_t debounceDelay = 50;
    Period period[sizeof(pinSensor)];
    Period interval;

    void count(byte pos)
    {
        if (period[pos].query() > data.counter.THRESHOLD)
        {
            period[pos].update();
            // ENABLE
            if (bitRead(data.counter.STATUS, 9))
            {
                data.counter.total[pos].TOTAL++;
                data.counter.total[pos].LAP = period[pos].getRPH();
                if (!bitRead(data.counter.STATUS, 10))
                    bitWrite(data.counter.STATUS, 11, 1);
                bitWrite(data.counter.STATUS, 10, 1);
                this->counting();
            }
            else
            {
                period[pos].reset();
            }
            interval.reset();
        }
    };

    void check()
    {
        // Verificamos si el contador se encuentra contando o detenido.
        if (bitRead(data.counter.STATUS, 10) && bitRead(data.counter.STATUS, 9))
        {
            if (interval.query() > data.counter.TIMEOUT * 1000UL)
            {
                bitWrite(data.counter.STATUS, 10, 0); // DETENIDO
                interval.reset();
                for (uint8_t i = 0; i < sizeof(pinSensor); i++)
                {
                    period[i].reset();
                    data.counter.total[i].LAP = 0;
                }
                bitWrite(data.counter.STATUS, 11, 1); // EVENTO 
                this->stopped();               
            }
        }
        else
        {
            interval.reset();
        }
    };

public:
    production_t data;

    Counter(){
        setDefault();
    };

    void begin(OnTick_t counting, OnTick_t stopped){
        this->counting = counting;
        this->stopped = stopped;
    };
    
    void init()
    {
        interval.enable();
        interval.reset();
        for (int i = 0; i < sizeof(pinSensor); i++)
        {
            pinMode(pinSensor[i], INPUT);
            period[i].enable();
            period[i].reset();
        }
    };

    void setDefault()
    {
        data.counter.STATUS = 0;
        data.counter.THRESHOLD = 1000;
        data.counter.TIMEOUT = 5;
    };
    void poll()
    {
        for (int i = 0; i < sizeof(pinSensor); i++)
        {
            int reading = digitalRead(pinSensor[i]);
            if (reading != debounceDIN[i].lastButtonState)
            {
                debounceDIN[i].lastDebounceTime = millis();
            }
            if ((millis() - debounceDIN[i].lastDebounceTime) > debounceDelay)
            {
                if (reading != debounceDIN[i].buttonState)
                {
                    debounceDIN[i].buttonState = reading;

                    if (debounceDIN[i].buttonState == HIGH)
                    {
                        count(i);
                    }
                }
            }
            debounceDIN[i].lastButtonState = reading;
        }
        check();
    }

    void reset()
    {
        interval.reset();
        for (uint8_t i = 0; i < sizeof(pinSensor); i++)
        {
            data.counter.total[i].TOTAL = 0;
            data.counter.total[i].LAP = 0;
            period[i].reset();
        }
    };
};

#endif