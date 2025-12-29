#include <Arduino.h>
#include <stdint.h>
#include "../../APP_Cfg.h"
#include "PWM.h"


#if PWM_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
#else
#define DEBUG_PRINTLN(var)
#endif


void PWM_initChannel(PWM_t* config) {
#if PWM_ENABLED == STD_ON
    DEBUG_PRINTLN("Initializing PWM Channel");
    analogWriteResolution(config->channel, config->resolution);
    DEBUG_PRINTLN("Attaching PWM Pin");
    analogWriteFrequency(config->channel, config->resolution); // Initialize with 0 duty cycle
#endif
}

void PWM_setDutyCycle(uint8_t channel, float dutyCyclePercentage) {
#if PWM_ENABLED == STD_ON
    DEBUG_PRINTLN("Setting Duty Cycle for PWM Channel: " + String(channel));
    uint32_t dutyCycle = (dutyCyclePercentage / 100.0) * ((1 << 8) - 1); // Assuming 8-bit resolution
    analogWrite(channel, dutyCycle);
    DEBUG_PRINTLN("Duty Cycle Set to: " + String(dutyCyclePercentage) + "%");
#endif
}