#include "sd_log.h"

#include "pff.h"
#include "hardware.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LOG_FILE "COLORLOG.CSV"

static FATFS fs;
static uint8_t sd_ready = 0;
static uint16_t log_count = 0;

static void build_log_line(char *line, const char *culoare, uint16_t red, uint16_t green, uint16_t blue) {
    char value[10];

    line[0] = '\0';

    strcat(line, culoare);
    strcat(line, ",");

    itoa(red, value, 10);
    strcat(line, value);
    strcat(line, ",");

    itoa(green, value, 10);
    strcat(line, value);
    strcat(line, ",");

    itoa(blue, value, 10);
    strcat(line, value);

    strcat(line, "\r\n");
}

uint8_t sd_log_init(void) {
    FRESULT result;
    UINT bytes_written;

    const char *header = "culoare,red,green,blue\r\n";

    sd_ready = 0;
    log_count = 0;

    result = pf_mount(&fs);
    if (result != FR_OK) {
        usart_print("\r\nSD: pf_mount error ");
        usart_print_uint(result);
        usart_print("\r\n");
        return result;
    }

    result = pf_open(LOG_FILE);
    if (result != FR_OK) {
        usart_print("\r\nSD: pf_open error ");
        usart_print_uint(result);
        usart_print("\r\n");
        return result;
    }

    // Scriem header-ul la inceputul fisierului
    // Dupa pf_write, pozitia interna din fisier ramane dupa header
    result = pf_write(header, strlen(header), &bytes_written);
    if (result != FR_OK) {
        usart_print("\r\nSD: header write error ");
        usart_print_uint(result);
        usart_print("\r\n");
        return result;
    }

    if (bytes_written != strlen(header)) {
        usart_print("\r\nSD: header incomplet\r\n");
        return 1;
    }

    result = pf_write(0, 0, &bytes_written);
    if (result != FR_OK) {
        usart_print("\r\nSD: header flush error ");
        usart_print_uint(result);
        usart_print("\r\n");
        return result;
    }

    sd_ready = 1;

    usart_print("\r\nSD: log init OK\r\n");

    return FR_OK;
}

uint8_t sd_log_detection(const char *culoare, uint16_t red, uint16_t green, uint16_t blue) {
    FRESULT result;
    UINT bytes_written;
    char line[64];
    uint16_t line_len;

    if (!sd_ready) {
        return 1;
    }

    build_log_line(line, culoare, red, green, blue);
    line_len = strlen(line);

    // Scriem direct de la pozitia curenta din fisier
    result = pf_write(line, line_len, &bytes_written);
    if (result != FR_OK) {
        sd_ready = 0;

        usart_print("\r\nSD: log write error ");
        usart_print_uint(result);
        usart_print("\r\n");

        return result;
    }

    if (bytes_written != line_len) {
        sd_ready = 0;

        usart_print("\r\nSD: log incomplet\r\n");

        return 1;
    }

    result = pf_write(0, 0, &bytes_written);
    if (result != FR_OK) {
        sd_ready = 0;

        usart_print("\r\nSD: log flush error ");
        usart_print_uint(result);
        usart_print("\r\n");

        return result;
    }

    log_count++;

    usart_print("SD log ");
    usart_print_uint(log_count);
    usart_print(": ");
    usart_print(line);

    return FR_OK;
}

uint8_t sd_log_is_ready(void) {
    return sd_ready;
}
