#ifndef APP_H
#define APP_H

#include <stdint.h>

extern volatile uint8_t operation_in_progress;

void app_init(void);
void app_handle_button(void);
void app_handle_usart_char(char c);

#endif
