#include "hardware.h"
#include "app.h"
#include "config.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdlib.h>

/* ------------------ TIMER0 ------------------ */

volatile uint32_t system_ms = 0;

void timer0_init(void) {
    TCCR0A = (1 << WGM01);
    TCCR0B = (1 << CS01) | (1 << CS00);
    OCR0A = 249;
    TIMSK0 = (1 << OCIE0A);
}

ISR(TIMER0_COMPA_vect) {
    system_ms++;
}

/* ------------------ BUTON ------------------ */

static volatile uint8_t button_pressed = 0;
static volatile uint32_t last_button_ms = 0;

void button_init(void) {
    DDRD &= ~(1 << BUTTON);
    PORTD |= (1 << BUTTON);

    EICRA |= (1 << ISC01);
    EICRA &= ~(1 << ISC00);
    EIMSK |= (1 << INT0);
}

ISR(INT0_vect) {
    if (!operation_in_progress && (system_ms - last_button_ms > 250)) {
        button_pressed = 1;
        last_button_ms = system_ms;
    }
}

uint8_t button_consume_press(void) {
    uint8_t pressed;
    uint8_t old_sreg = SREG;

    cli();
    pressed = button_pressed;
    button_pressed = 0;
    SREG = old_sreg;

    return pressed;
}

void button_clear_press(void) {
    uint8_t old_sreg = SREG;

    cli();
    button_pressed = 0;
    SREG = old_sreg;
}

/* ------------------ USART ------------------ */

void usart_init(void) {
    UBRR0H = (uint8_t)(UBRR_VALUE >> 8);
    UBRR0L = (uint8_t)UBRR_VALUE;

    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void usart_send_char(char c) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

void usart_print(const char *text) {
    while (*text) {
        usart_send_char(*text++);
    }
}

uint8_t usart_available(void) {
    return (UCSR0A & (1 << RXC0));
}

char usart_read_char(void) {
    return UDR0;
}

void usart_print_uint(uint16_t value) {
    char buffer[10];

    itoa(value, buffer, 10);
    usart_print(buffer);
}

void usart_show_menu(void) {
    usart_print("\r\n============================\r\n");
    usart_print("ColorTrack\r\n");
    usart_print("============================\r\n");
    usart_print("1 - Modul detectie simpla\r\n");
    usart_print("2 - Modul cautare culoare\r\n");
    usart_print("M - Afisare meniu\r\n");
    usart_print("\r\nModul 1: alegi 1, apoi apesi butonul.\r\n");
    usart_print("Modul 2: alegi 2, trimiti R/G/B, apoi apesi butonul.\r\n");
    usart_print("> ");
}

/* ------------------ TWI / I2C ------------------ */

void twi_init(void) {
    TWSR = 0x00;
    TWBR = 72;
}

void twi_start(void) {
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

void twi_stop(void) {
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

void twi_write(uint8_t data) {
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

/* ------------------ LCD ------------------ */

static void lcd_i2c_write(uint8_t data) {
    twi_start();
    twi_write((LCD_ADDR << 1) | 0);
    twi_write(data | LCD_BACKLIGHT);
    twi_stop();
}

static void lcd_pulse_enable(uint8_t data) {
    lcd_i2c_write(data | LCD_ENABLE);
    _delay_us(1);

    lcd_i2c_write(data & ~LCD_ENABLE);
    _delay_us(50);
}

static void lcd_write4bits(uint8_t data) {
    lcd_i2c_write(data);
    lcd_pulse_enable(data);
}

static void lcd_send(uint8_t value, uint8_t mode) {
    uint8_t high_nibble = value & 0xF0;
    uint8_t low_nibble = (value << 4) & 0xF0;

    lcd_write4bits(high_nibble | mode);
    lcd_write4bits(low_nibble | mode);
}

static void lcd_command(uint8_t command) {
    lcd_send(command, 0);
}

static void lcd_data(uint8_t data) {
    lcd_send(data, LCD_RS);
}

void lcd_init(void) {
    _delay_ms(50);

    lcd_write4bits(0x30);
    _delay_ms(5);

    lcd_write4bits(0x30);
    _delay_us(150);

    lcd_write4bits(0x30);
    _delay_us(150);

    lcd_write4bits(0x20);
    _delay_us(150);

    lcd_command(0x28);
    lcd_command(0x0C);
    lcd_command(0x01);
    _delay_ms(2);
    lcd_command(0x06);
}

void lcd_clear(void) {
    lcd_command(0x01);
    _delay_ms(2);
}

void lcd_set_cursor(uint8_t row, uint8_t col) {
    uint8_t address = (row == 0) ? col : (0x40 + col);

    lcd_command(0x80 | address);
}

void lcd_print(const char *text) {
    while (*text) {
        lcd_data(*text++);
    }
}

void lcd_print_uint(uint16_t value) {
    char buffer[10];

    itoa(value, buffer, 10);
    lcd_print(buffer);
}

/* ------------------ PWM SOFTWARE + LED RGB ------------------ */

static volatile uint8_t pwm_counter = 0;
static volatile uint8_t pwm_r = 0;
static volatile uint8_t pwm_g = 0;
static volatile uint8_t pwm_b = 0;

void soft_pwm_init(void) {
    TCCR2A = (1 << WGM21);
    TCCR2B = (1 << CS21);
    OCR2A = 63;
    TIMSK2 = (1 << OCIE2A);
}

ISR(TIMER2_COMPA_vect) {
    uint8_t output = 0;

    pwm_counter++;

    if (pwm_counter < pwm_r) {
        output |= (1 << LED_R);
    }

    if (pwm_counter < pwm_g) {
        output |= (1 << LED_G);
    }

    if (pwm_counter < pwm_b) {
        output |= (1 << LED_B);
    }

    PORTD = (PORTD & ~LED_MASK) | output;
}

void rgb_set_pwm(uint8_t r, uint8_t g, uint8_t b) {
    pwm_r = r;
    pwm_g = g;
    pwm_b = b;
}

void rgb_off(void) {
    rgb_set_pwm(0, 0, 0);
}

void rgb_red(void) {
    rgb_set_pwm(255, 0, 0);
}

void rgb_green(void) {
    rgb_set_pwm(0, 255, 0);
}

void rgb_blue(void) {
    rgb_set_pwm(0, 0, 255);
}

void rgb_white(void) {
    rgb_set_pwm(255, 255, 255);
}

void rgb_init(void) {
    DDRD |= LED_MASK;
    rgb_off();
}

void rgb_set_color(color_t color) {
    if (color == COLOR_RED) {
        rgb_red();
    } else if (color == COLOR_GREEN) {
        rgb_green();
    } else if (color == COLOR_BLUE) {
        rgb_blue();
    } else {
        rgb_white();
    }
}

static uint8_t raw_to_pwm_inverse(uint16_t value, uint16_t min_value, uint16_t max_value) {
    if (max_value <= min_value) {
        return 0;
    }

    if (value <= min_value) {
        return 255;
    }

    if (value >= max_value) {
        return 0;
    }

    return (uint8_t)(((uint32_t)(max_value - value) * 255UL) / (max_value - min_value));
}

void rgb_set_from_raw(uint16_t red, uint16_t green, uint16_t blue) {
    uint16_t min_value = red;
    uint16_t max_value = red;

    if (green < min_value) {
        min_value = green;
    }

    if (blue < min_value) {
        min_value = blue;
    }

    if (green > max_value) {
        max_value = green;
    }

    if (blue > max_value) {
        max_value = blue;
    }

    if ((max_value - min_value) < 20) {
        rgb_white();
        return;
    }

    rgb_set_pwm(
        raw_to_pwm_inverse(red, min_value, max_value),
        raw_to_pwm_inverse(green, min_value, max_value),
        raw_to_pwm_inverse(blue, min_value, max_value)
    );
}

/* ------------------ MOTOR ------------------ */

static uint8_t motor_step_index = 0;

void motor_init(void) {
    DDRC |= (1 << M_IN1) | (1 << M_IN2);
    DDRB |= (1 << M_IN3);
    DDRD |= (1 << M_IN4);
}

void motor_off(void) {
    PORTC &= ~((1 << M_IN1) | (1 << M_IN2));
    PORTB &= ~(1 << M_IN3);
    PORTD &= ~(1 << M_IN4);
}

static void motor_set_step(uint8_t step) {
    motor_off();

    switch (step) {
        case 0:
            PORTC |= (1 << M_IN1);
            break;

        case 1:
            PORTC |= (1 << M_IN1) | (1 << M_IN2);
            break;

        case 2:
            PORTC |= (1 << M_IN2);
            break;

        case 3:
            PORTC |= (1 << M_IN2);
            PORTB |= (1 << M_IN3);
            break;

        case 4:
            PORTB |= (1 << M_IN3);
            break;

        case 5:
            PORTB |= (1 << M_IN3);
            PORTD |= (1 << M_IN4);
            break;

        case 6:
            PORTD |= (1 << M_IN4);
            break;

        case 7:
            PORTD |= (1 << M_IN4);
            PORTC |= (1 << M_IN1);
            break;
    }
}

static void motor_step_forward(void) {
    motor_set_step(motor_step_index);

    motor_step_index++;
    if (motor_step_index >= 8) {
        motor_step_index = 0;
    }

    _delay_ms(MOTOR_STEP_DELAY);
}

static void motor_step_backward(void) {
    if (motor_step_index == 0) {
        motor_step_index = 7;
    } else {
        motor_step_index--;
    }

    motor_set_step(motor_step_index);
    _delay_ms(MOTOR_STEP_DELAY);
}

void motor_rotate_forward(uint16_t steps) {
    for (uint16_t i = 0; i < steps; i++) {
        motor_step_forward();
    }

    motor_off();
}

void motor_rotate_backward(uint16_t steps) {
    for (uint16_t i = 0; i < steps; i++) {
        motor_step_backward();
    }

    motor_off();
}

/* ------------------ TCS230 ------------------ */

void tcs_init(void) {
    DDRC |= (1 << TCS_S2) | (1 << TCS_S3);

    DDRB &= ~(1 << TCS_OUT);
    PORTB &= ~(1 << TCS_OUT);

    TCCR1A = 0;
    TCCR1B = (1 << CS11);
    TCNT1 = 0;
}

static void tcs_select_red(void) {
    PORTC &= ~(1 << TCS_S2);
    PORTC &= ~(1 << TCS_S3);
}

static void tcs_select_blue(void) {
    PORTC &= ~(1 << TCS_S2);
    PORTC |= (1 << TCS_S3);
}

static void tcs_select_green(void) {
    PORTC |= (1 << TCS_S2);
    PORTC |= (1 << TCS_S3);
}

static uint16_t measure_high_pulse(void) {
    uint16_t timeout = 60000;

    TCNT1 = 0;

    while (PINB & (1 << TCS_OUT)) {
        if (TCNT1 > timeout) {
            return 0;
        }
    }

    TCNT1 = 0;

    while (!(PINB & (1 << TCS_OUT))) {
        if (TCNT1 > timeout) {
            return 0;
        }
    }

    TCNT1 = 0;

    while (PINB & (1 << TCS_OUT)) {
        if (TCNT1 > timeout) {
            return 0;
        }
    }

    return TCNT1;
}

static uint16_t read_color_value(void (*select_filter)(void)) {
    uint32_t sum = 0;
    uint8_t valid = 0;

    select_filter();
    _delay_ms(40);

    for (uint8_t i = 0; i < 5; i++) {
        uint16_t value = measure_high_pulse();

        if (value != 0) {
            sum += value;
            valid++;
        }

        _delay_ms(3);
    }

    if (valid == 0) {
        return 0;
    }

    return (uint16_t)(sum / valid);
}

const char *color_to_text(color_t color) {
    if (color == COLOR_RED) {
        return "ROSU";
    }

    if (color == COLOR_GREEN) {
        return "VERDE";
    }

    if (color == COLOR_BLUE) {
        return "ALBASTRU";
    }

    return "NECUNOSCUT";
}

static color_t detect_color(uint16_t red, uint16_t green, uint16_t blue) {
    if (red == 0 || green == 0 || blue == 0) {
        return COLOR_UNKNOWN;
    }

    if (red < green && red < blue) {
        return COLOR_RED;
    }

    if (green < red && green < blue) {
        return COLOR_GREEN;
    }

    if (blue < red && blue < green) {
        return COLOR_BLUE;
    }

    return COLOR_UNKNOWN;
}

color_t read_detected_color(uint16_t *red, uint16_t *green, uint16_t *blue) {
    *red = read_color_value(tcs_select_red);
    *green = read_color_value(tcs_select_green);
    *blue = read_color_value(tcs_select_blue);

    return detect_color(*red, *green, *blue);
}
