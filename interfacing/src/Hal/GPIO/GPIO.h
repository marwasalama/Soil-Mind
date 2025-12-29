#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
typedef struct {
    uint8_t pin;
    uint8_t mode; // INPUT, OUTPUT, INPUT_PULLUP
} GPIO_t;


void GPIO_initPin(uint8_t pin, uint8_t mode);

void GPIO_writePin(uint8_t pin, uint8_t value);

uint8_t GPIO_readPin(uint8_t pin);



#endif // GPIO_H