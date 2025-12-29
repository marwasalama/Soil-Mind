#ifndef APP_CFG_H
#define APP_CFG_H 

//General Definitions
#define STD_ON 1
#define STD_OFF 0
#define WiFi    1
#define BLE     2
#define _4G    3

//Module definitions
#define GPIO_ENABLED               STD_OFF
#define COMMUNICATION_MODULE       WiFi
#define SENSORH_ENABLED            STD_OFF
#define POT_ENABLED                STD_OFF
#define LM35_ENABLED               STD_OFF
#define PWM_ENABLED                STD_OFF
#define DIMALARM_ENABLED           STD_OFF
#define UART_ENABLED               STD_ON
#define ChatApp_ENABLED            STD_ON

//Debug Definitions
#define GPIO_DEBUG                 STD_OFF
#define SENSORH_DEBUG              STD_OFF
#define POT_DEBUG                  STD_OFF
#define LM35_DEBUG                 STD_OFF
#define PWM_DEBUG                  STD_OFF
#define DIMALARM_DEBUG             STD_OFF
#define UART_DEBUG                 STD_ON
#define ChatApp_DEBUG              STD_ON

//Pin Configuration
#define POT_PIN             34
#define LM35_PIN            0
#define POT_RESOLUTION      12
#define LM35_RESOLUTION     10
#define ALARM_LOW_LED       16
#define ALARM_HIGH_LED      17
#define DIMER_LED           9

//General Configurations
#define MAX_TEMP_RANGE                   150.0// Maximum temperature range for LM35 sensor in Celsius
#define ALARM_LOW_THRESHOLD_PERCENTAGE   20.0 // Low Voltage threshold for DimAlarm in Celsius
#define ALARM_HIGH_THRESHOLD_PERCENTAGE  80.0 // High Voltage threshold for DimAlarm in Celsius
#define ADC_MAX_VALUE                    4095 // 12-bit ADC

//UART1 Configuration
#define UART1_BAUD_RATE 115200
#define UART1_TX_PIN    17
#define UART1_RX_PIN    16
#define UART1_FRAME_CFG SERIAL_8N1

#endif
