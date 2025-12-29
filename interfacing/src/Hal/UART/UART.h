#ifndef UART_H
#define UART_H


#include <Arduino.h>
#include <stdint.h>

typedef enum {
    UART1 = 0, // Using index 0
    MAXLENGTH // Total number of defined UARTs
} UARTN_t;

typedef struct{
    uint32_t baud_rate;
    uint8_t tx;
    uint8_t rx;
    uint32_t frame_cfg;
}UARCfg_t;

void UART_init(void);

void UART_read(UARTN_t uart_n, String& payload);

void UART_write(UARTN_t uart_n, const char* payload);


#endif