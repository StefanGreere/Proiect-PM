#ifndef SD_LOG_H
#define SD_LOG_H

#include <stdint.h>

uint8_t sd_log_init(void);
uint8_t sd_log_detection(const char *culoare, uint16_t red, uint16_t green, uint16_t blue);
uint8_t sd_log_is_ready(void);

#endif
