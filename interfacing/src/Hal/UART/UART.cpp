#include <Arduino.h>
#include <stdint.h>
#include "../../APP_Cfg.h"
#include "UART.h" 


#if UART_DEBUG == STD_ON
#define DEBUG_PRINTLN(var) Serial.println(var)
#else
#define DEBUG_PRINTLN(var)
#endif

static UARCfg_t UART[MAXLENGTH] = {
    {UART1_BAUD_RATE, UART1_TX_PIN, UART1_RX_PIN, UART1_FRAME_CFG}
};


static HardwareSerial myserials[MAXLENGTH] = {Serial1};

void UART_init(void)
{
#if UART_ENABLED==STD_ON
    for(int i=0;i<MAXLENGTH;i++)
    {
        myserials[i].begin(UART[i].baud_rate,UART[i].frame_cfg,UART[i].rx,UART[i].tx);
        DEBUG_PRINTLN("UART Initialized with Baud Rate: " + String(UART[i].baud_rate));
    }
#endif
}

void UART_read(UARTN_t uart_n,String& payload)
{
#if UART_ENABLED==STD_ON
    if (myserials[uart_n].available()) {
        payload = myserials[uart_n].readStringUntil('\n');
        DEBUG_PRINTLN("Received Payload: " + payload);
    }
#endif
}

void UART_write(UARTN_t uart_n,const char* payload)
{
#if UART_ENABLED==STD_ON
    myserials[uart_n].println(payload);
    DEBUG_PRINTLN("Sent Payload: " + String(payload));
#endif
}


