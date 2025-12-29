#ifndef SENSORH_H
#define SENSORH_H
#include <stdint.h>


typedef struct{
    uint8_t channel;
    uint8_t resolution;
}SensorH_t;



void SensorH_Init(SensorH_t* config);

uint32_t SensorH_ReadValue(uint8_t channel);

#endif 