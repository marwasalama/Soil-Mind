#ifndef PWM_H
#define PWM_H


#include <stdint.h>

typedef struct {
    uint8_t channel;
    uint32_t frequency;
    uint8_t resolution;
} PWM_t;


void PWM_initChannel(PWM_t* config);


void PWM_setDutyCycle(uint8_t channel, float dutyCyclePercentage);


#endif // PWM_H