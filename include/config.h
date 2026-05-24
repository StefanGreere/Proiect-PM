#ifndef CONFIG_H
#define CONFIG_H

#include <avr/io.h>

/* LCD I2C */
#define LCD_ADDR      0x27
#define LCD_BACKLIGHT 0x08
#define LCD_ENABLE    0x04
#define LCD_RS        0x01

/* LED RGB */
#define LED_B PD3
#define LED_G PD5
#define LED_R PD6
#define LED_MASK ((1 << LED_R) | (1 << LED_G) | (1 << LED_B))

/* Buton INT0 */
#define BUTTON PD2

/* microSD */
#define SD_CS PB2

/* TCS230: S0 -> 5V, S1 -> GND */
#define TCS_S2  PC0
#define TCS_S3  PC1
#define TCS_OUT PB0

/* Motor ULN2003 */
#define M_IN1 PC2
#define M_IN2 PC3
#define M_IN3 PB1
#define M_IN4 PD4

#define SCAN_SIDE_LIMIT    1000
#define SCAN_CHUNK_STEPS   64
#define MOTOR_STEP_DELAY   3

/* USART */
#define BAUD 9600UL
#define UBRR_VALUE ((F_CPU / (16UL * BAUD)) - 1)

#endif
