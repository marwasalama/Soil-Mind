#include <Arduino.h>
#include <stdint.h>

#include "../../APP_Cfg.h"
#include "GPIO.h"




#if GPIO_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
#else
#define DEBUG_PRINTLN(var)
#endif



void GPIO_initPin(uint8_t pin, uint8_t mode) {
#if GPIO_ENABLED == STD_ON
    pinMode(pin, mode);
    DEBUG_PRINTLN("Initialized GPIO Pin: " + String(pin) + " with mode: " + String(mode));
#endif
}



void GPIO_writePin(uint8_t pin, uint8_t value) {
#if GPIO_ENABLED == STD_ON
    digitalWrite(pin, value);
    DEBUG_PRINTLN("Wrote value: " + String(value) + " to GPIO Pin: " + String(pin));
#endif
}


uint8_t GPIO_readPin(uint8_t pin) {
#if GPIO_ENABLED == STD_ON
    uint8_t value = digitalRead(pin);
    DEBUG_PRINTLN("Read value: " + String(value) + " from GPIO Pin: " + String(pin));
    return value;
#else
    return 0; // Default return if GPIO is disabled
#endif 
}