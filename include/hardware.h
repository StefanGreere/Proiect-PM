#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdint.h>
#include "types.h"

extern volatile uint32_t system_ms;

void timer0_init(void);

void button_init(void);
uint8_t button_consume_press(void);
void button_clear_press(void);

void usart_init(void);
void usart_send_char(char c);
void usart_print(const char *text);
void usart_print_uint(uint16_t value);
uint8_t usart_available(void);
char usart_read_char(void);
void usart_show_menu(void);

void twi_init(void);
void twi_start(void);
void twi_stop(void);
void twi_write(uint8_t data);

void lcd_init(void);
void lcd_clear(void);
void lcd_set_cursor(uint8_t row, uint8_t col);
void lcd_print(const char *text);
void lcd_print_uint(uint16_t value);

void soft_pwm_init(void);
void rgb_set_pwm(uint8_t r, uint8_t g, uint8_t b);
void rgb_init(void);
void rgb_off(void);
void rgb_red(void);
void rgb_green(void);
void rgb_blue(void);
void rgb_white(void);
void rgb_set_color(color_t color);
void rgb_set_from_raw(uint16_t red, uint16_t green, uint16_t blue);

void motor_init(void);
void motor_off(void);
void motor_rotate_forward(uint16_t steps);
void motor_rotate_backward(uint16_t steps);

void tcs_init(void);
const char *color_to_text(color_t color);
color_t read_detected_color(uint16_t *red, uint16_t *green, uint16_t *blue);

#endif
