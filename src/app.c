#include "app.h"
#include "config.h"
#include "hardware.h"
#include "sd_log.h"

#include <util/delay.h>
#include <stdint.h>

/* ------------------ STARE APLICATIE ------------------ */

static uint8_t waiting_for_target = 0;
static mode_t current_mode = MODE_NONE;
static color_t current_target = COLOR_BLUE;
volatile uint8_t operation_in_progress = 0;

/* ------------------ LCD helpers ------------------ */

static void lcd_show_menu(void) {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("ColorTrack");
    lcd_set_cursor(1, 0);
    lcd_print("USART: 1 sau 2");
}

static void lcd_show_mode_1_waiting(void) {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Mod 1: detectie");
    lcd_set_cursor(1, 0);
    lcd_print("Apasa buton");
}

static void lcd_show_mode_2_choose_target(void) {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Mod 2: cautare");
    lcd_set_cursor(1, 0);
    lcd_print("Tinta: R/G/B");
}

static void lcd_show_target(color_t target) {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Tinta setata:");
    lcd_set_cursor(1, 0);
    lcd_print(color_to_text(target));
}

static void lcd_show_scanning_side(color_t target, const char *side, uint16_t steps) {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Caut ");
    lcd_print(color_to_text(target));
    lcd_set_cursor(1, 0);
    lcd_print(side);
    lcd_print(" ");
    lcd_print_uint(steps);
}

static void lcd_show_found(color_t target, uint16_t steps) {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print(color_to_text(target));
    lcd_print(" gasit");
    lcd_set_cursor(1, 0);
    lcd_print("Pas:");
    lcd_print_uint(steps);
}

static void lcd_show_returning(void) {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Revenire");
    lcd_set_cursor(1, 0);
    lcd_print("pozitie start");
}

static void lcd_show_return_done_found(color_t target) {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print(color_to_text(target));
    lcd_print(" gasit");
    lcd_set_cursor(1, 0);
    lcd_print("Pozitie start");
}

static void lcd_show_not_found(void) {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Culoare");
    lcd_set_cursor(1, 0);
    lcd_print("negasita");
}

static void lcd_show_return_done_not_found(void) {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Negasit");
    lcd_set_cursor(1, 0);
    lcd_print("Pozitie start");
}

/* ------------------ CONVERSIE COMENZI ------------------ */

static color_t char_to_color(char c) {
    if (c == 'R' || c == 'r') {
        return COLOR_RED;
    }

    if (c == 'G' || c == 'g' || c == 'V' || c == 'v') {
        return COLOR_GREEN;
    }

    if (c == 'B' || c == 'b' || c == 'A' || c == 'a') {
        return COLOR_BLUE;
    }

    return COLOR_UNKNOWN;
}

/* ------------------ MOD 1 ------------------ */

static const char *detect_display_color(color_t base_color, uint16_t red, uint16_t green, uint16_t blue) {
    uint16_t diff_rg;

    if (red > green) {
        diff_rg = red - green;
    } else {
        diff_rg = green - red;
    }

    /* Galben: rosu si verde apropiate, albastru slab. */
    if (blue > red + 60 && blue > green + 60 && diff_rg < 12) {
        return "GALBEN";
    }

    /* Portocaliu: rosu puternic, verde mediu, albastru slab. */
    if (red < green && green < blue) {
        if (green <= red * 3) {
            return "PORTOCALIU";
        }
    }

    /* Roz: rosu puternic, albastru prezent, verde slab. */
    if (red < blue && blue < green) {
        if (blue <= red * 2) {
            return "ROZ";
        }
    }

    /* Mov: albastru puternic, rosu prezent, verde slab. */
    if (blue < red && red < green) {
        if (red <= blue * 2) {
            return "MOV";
        }
    }

    return color_to_text(base_color);
}

static void perform_simple_detection(void) {
    uint16_t red;
    uint16_t green;
    uint16_t blue;

    operation_in_progress = 1;

    color_t color = read_detected_color(&red, &green, &blue);
    const char *display_color = detect_display_color(color, red, green, blue);

    rgb_set_from_raw(red, green, blue);

    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Detectat:");
    lcd_set_cursor(1, 0);
    lcd_print(display_color);

    sd_log_detection(display_color, red, green, blue);

    usart_print("\r\nDetectie simpla:\r\n");
    usart_print("R=");
    usart_print_uint(red);
    usart_print(" G=");
    usart_print_uint(green);
    usart_print(" B=");
    usart_print_uint(blue);
    usart_print(" => ");
    usart_print(display_color);
    usart_print("\r\nApasa din nou butonul pentru o noua detectie.\r\n> ");

    operation_in_progress = 0;
}

/* ------------------ MOD 2 ------------------ */

static void finish_found_left(color_t target, uint16_t steps_done) {
    lcd_show_found(target, steps_done);

    usart_print("Culoare gasita la stanga, pasul ");
    usart_print_uint(steps_done);
    usart_print("\r\n");

    rgb_set_color(target);
    _delay_ms(1000);

    lcd_show_returning();
    usart_print("Revenire din stanga la pozitia initiala...\r\n");

    motor_rotate_backward(steps_done);
    motor_off();

    lcd_show_return_done_found(target);
    rgb_set_color(target);
}

static void finish_found_right(color_t target, uint16_t steps_done) {
    lcd_show_found(target, steps_done);

    usart_print("Culoare gasita la dreapta, pasul ");
    usart_print_uint(steps_done);
    usart_print("\r\n");

    rgb_set_color(target);
    _delay_ms(1000);

    lcd_show_returning();
    usart_print("Revenire din dreapta la pozitia initiala...\r\n");

    motor_rotate_forward(steps_done);
    motor_off();

    lcd_show_return_done_found(target);
    rgb_set_color(target);
}

static void perform_scan_for_color(color_t target) {
    uint16_t red;
    uint16_t green;
    uint16_t blue;
    color_t detected;
    uint16_t steps_done;
    uint16_t step_now;

    operation_in_progress = 1;
    rgb_white();

    usart_print("\r\nPornesc scanarea stanga-dreapta pentru ");
    usart_print(color_to_text(target));
    usart_print("\r\n");

    _delay_ms(500);

    /* Cautare la stanga. */
    steps_done = 0;

    while (steps_done < SCAN_SIDE_LIMIT) {
        if ((SCAN_SIDE_LIMIT - steps_done) < SCAN_CHUNK_STEPS) {
            step_now = SCAN_SIDE_LIMIT - steps_done;
        } else {
            step_now = SCAN_CHUNK_STEPS;
        }

        motor_rotate_forward(step_now);
        steps_done += step_now;

        lcd_show_scanning_side(target, "Stanga", steps_done);
        detected = read_detected_color(&red, &green, &blue);
        rgb_white();

        if (detected == target) {
            finish_found_left(target, steps_done);
            usart_print("Gata. Poti apasa din nou butonul sau poti selecta alt mod.\r\n> ");
            button_clear_press();
            operation_in_progress = 0;
            return;
        }
    }

    motor_off();
    lcd_show_returning();
    usart_print("Nu am gasit la stanga. Revin la pozitia initiala...\r\n");

    motor_rotate_backward(steps_done);
    motor_off();
    _delay_ms(500);

    /* Cautare la dreapta. */
    steps_done = 0;

    while (steps_done < SCAN_SIDE_LIMIT) {
        if ((SCAN_SIDE_LIMIT - steps_done) < SCAN_CHUNK_STEPS) {
            step_now = SCAN_SIDE_LIMIT - steps_done;
        } else {
            step_now = SCAN_CHUNK_STEPS;
        }

        motor_rotate_backward(step_now);
        steps_done += step_now;

        lcd_show_scanning_side(target, "Dreapta", steps_done);
        detected = read_detected_color(&red, &green, &blue);
        rgb_white();

        if (detected == target) {
            finish_found_right(target, steps_done);
            usart_print("Gata. Poti apasa din nou butonul sau poti selecta alt mod.\r\n> ");
            button_clear_press();
            operation_in_progress = 0;
            return;
        }
    }

    motor_off();
    lcd_show_not_found();

    usart_print("Culoare negasita nici la stanga, nici la dreapta.\r\n");
    rgb_red();
    _delay_ms(1000);

    lcd_show_returning();
    usart_print("Revenire finala la pozitia initiala...\r\n");

    motor_rotate_forward(steps_done);
    motor_off();

    lcd_show_return_done_not_found();
    rgb_red();

    usart_print("Gata. Poti apasa din nou butonul sau poti selecta alt mod.\r\n> ");

    button_clear_press();
    operation_in_progress = 0;
}

/* ------------------ APP ------------------ */

void app_init(void) {
    lcd_show_menu();
    usart_show_menu();
}

void app_handle_button(void) {
    if (waiting_for_target) {
        usart_print("\r\nAlege mai intai culoarea tinta: R, G sau B.\r\n> ");
        lcd_show_mode_2_choose_target();
    } else if (current_mode == MODE_SIMPLE_DETECTION) {
        perform_simple_detection();
    } else if (current_mode == MODE_SCAN_COLOR) {
        perform_scan_for_color(current_target);
    } else {
        usart_print("\r\nAlege mai intai modul 1 sau 2.\r\n> ");
        lcd_show_menu();
    }
}

void app_handle_usart_char(char c) {
    color_t target;

    if (c == '\r' || c == '\n') {
        return;
    }

    if (waiting_for_target) {
        target = char_to_color(c);

        if (target != COLOR_UNKNOWN) {
            current_target = target;
            waiting_for_target = 0;

            rgb_white();
            lcd_show_target(target);

            usart_print("\r\nCuloare tinta setata: ");
            usart_print(color_to_text(target));
            usart_print("\r\nApasa butonul pentru a incepe scanarea.\r\n> ");
        } else {
            usart_print("\r\nCuloare invalida. Trimite R, G sau B.\r\n> ");
        }

        return;
    }

    if (c == '1') {
        current_mode = MODE_SIMPLE_DETECTION;
        waiting_for_target = 0;

        rgb_white();
        lcd_show_mode_1_waiting();

        usart_print("\r\nMod 1 selectat: detectie simpla.\r\n");
        usart_print("Apasa butonul pentru detectie.\r\n> ");
    } else if (c == '2') {
        current_mode = MODE_SCAN_COLOR;
        waiting_for_target = 1;

        rgb_white();
        lcd_show_mode_2_choose_target();

        usart_print("\r\nMod 2 selectat: cautare culoare.\r\n");
        usart_print("Trimite R, G sau B pentru culoarea tinta:\r\n> ");
    } else if (c == 'm' || c == 'M') {
        waiting_for_target = 0;
        current_mode = MODE_NONE;

        rgb_off();
        lcd_show_menu();
        usart_show_menu();
    } else {
        usart_print("\r\nComanda necunoscuta. Trimite 1, 2 sau M.\r\n> ");
    }
}
