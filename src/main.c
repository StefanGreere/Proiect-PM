#include "app.h"
#include "hardware.h"
#include "sd_log.h"

#include <avr/interrupt.h>
#include <util/delay.h>

int main(void) {
    rgb_init();
    soft_pwm_init();

    motor_init();
    motor_off();

    twi_init();
    lcd_init();

    timer0_init();
    tcs_init();
    button_init();

    usart_init();

    while (sd_log_init() != 0) {
        usart_print("\r\nSD nu este gata.\r\n");
        usart_print("Scoate cardul, baga-l la loc si asteapta...\r\n");
        usart_print("Reincerc in 2 secunde.\r\n");

        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print("SD nu e gata");
        lcd_set_cursor(1, 0);
        lcd_print("Reintrodu card");

        _delay_ms(2000);
    }

    usart_print("\r\nSD este gata. Pornesc meniul.\r\n");

    sei();

    app_init();

    while (1) {
        if (button_consume_press()) {
            app_handle_button();
        }

        if (usart_available()) {
            app_handle_usart_char(usart_read_char());
        }
    }

    return 0;
}
