/**
 * CPE/CSC 159 Operating System Pragmatics
 * California State University, Sacramento
 *
 * Operating system entry point
 */

#include <spede/stdlib.h>
#include "vga.h"
#include "keyboard.h"
#include "bit.h"
#include "kernel.h"

void main(void) {

    // Initialize core drivers
    vga_init();
    keyboard_init();

    // Startup message
    vga_printf("Welcome to %s!\n", OS_NAME);

    // Exercise bit utilities
    int value = 0xdecafbad;
    int count = bit_count(value);
    int test  = bit_test(value, count);

    int toggle1 = 31;
    int toggle2 = 0;

    int value1 = bit_toggle(value, toggle1);
    int value2 = bit_toggle(value, toggle2);

    vga_printf("0x%08x has %d bits set\n", value, count);
    vga_printf("bit %d of 0x%08x is %d\n", count, value, test);
    vga_printf("0x%08x -> toggling bit %d = 0x%08x\n", value, toggle1, value1);
    vga_printf("0x%08x -> toggling bit %d = 0x%08x\n", value, toggle2, value2);

    vga_printf("Press any key to test VGA output\n");
    // keyboard_getc();

    // Display ASCII characters
    for (int c = 0; c < 255; c++) {
        vga_putc((unsigned char)c);
    }

    vga_printf("Press any key to test VGA colors\n");
    // keyboard_getc();

    // Color test grid
    for (int x = 0; x <= 0xF; x++) {
        for (int y = 0; y <= 0xF; y++) {
            vga_putc_at(VGA_HEIGHT - y - 2,
                        VGA_WIDTH  - x - 2,
                        y, x, '*');
        }
    }

    // Help text
    vga_puts_at(VGA_HEIGHT - 1, 0,
                VGA_COLOR_LIGHT_BLUE,
                VGA_COLOR_WHITE,
                "CTRL-K to clear screen, CTRL-ESC x 3 to exit");

    vga_puts_at(VGA_HEIGHT - 2, 0,
                VGA_COLOR_CYAN,
                VGA_COLOR_WHITE,
                "CTRL-P to test panic, CTRL-B for breakpoint");

    vga_set_bg(VGA_COLOR_BLACK);
    vga_set_fg(VGA_COLOR_LIGHT_GREY);

    vga_printf("\nPress any key to test keyboard input\n");
    // keyboard_getc();

    // Keyboard echo loop
    while (1) {
        unsigned int c = keyboard_poll();
        char buf[80] = {0};

        if (c != KEY_NULL) {
            if (c == KEY_ESCAPE || c == 0x02 || c == 0x0B || c == 0x10 || c == 0x16) {
                kernel_command((char)c);
            } else {
                vga_putc((unsigned char)c);
            }

            snprintf(buf, sizeof(buf),
                     "%02d, %02d",
                     vga_get_row(),
                     vga_get_col());

            vga_puts_at(VGA_HEIGHT - 1,
                        74,
                        VGA_COLOR_LIGHT_RED,
                        VGA_COLOR_WHITE,
                        buf);
        }
    }
}