/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 *
 * TTY Definitions
 */

#include <spede/string.h>

#include "kernel.h"
#include "timer.h"
#include "tty.h"
#include "vga.h"

// TTY Table
struct tty_t tty_table[TTY_MAX];

// Current Active TTY
struct tty_t *active_tty;

/**
 * Sets the active TTY to the selected TTY number
 * @param tty - TTY number
 */
void tty_select(int n) {
    if (n < 0 || n >= TTY_MAX) {
        kernel_panic("Invalid TTY %d", n);
    }

    active_tty = &tty_table[n];
    kernel_log_info("tty[%d]: selected", n);

    active_tty->refresh = 1;
}

/**
 * Returns the active TTY identifier
 * @return TTY id or -1 on error
 */
int tty_get_active(void) {
    if (!active_tty) {
        return -1;
    }

    return active_tty->id;
}

/**
 * Returns the tty structure for the given tty number
 * @param tty - TTY number
 * @return tty pointer
 */
struct tty_t *tty_get(int tty) {
    if (tty < 0 || tty >= TTY_MAX) {
        kernel_panic("Invalid TTY %d", tty);
        return NULL;
    }

    return &tty_table[tty];
}

/**
 * Refreshes the tty if needed
 */
void tty_refresh(void) {
    if (!active_tty) {
        kernel_panic("No TTY is selected!");
        return;
    }

    struct tty_t *tty = active_tty;
    char c;

    // Handle new output
    while (!ringbuf_is_empty(&tty->io_output)) {
        if (ringbuf_read(&tty->io_output, &c) == 0) {
            tty_update(c);
        }
    }

    if (tty->refresh) {
        kernel_log_trace("tty[%d]: refreshing", tty->id);

        int x = 0;
        int y = 0;

        for (int i = 0; i < TTY_WIDTH * TTY_HEIGHT; i++) {
            if (x >= VGA_WIDTH) {
                x = 0;
                y++;
            }

            vga_putc_at(
                x++,
                y,
                tty->color_bg,
                tty->color_fg,
                tty->buf[tty->pos_scroll * TTY_WIDTH + i]
            );
        }

        tty->refresh = 0;
    }
}

/**
 * Write a character into the TTY process input buffer
 * If the echo flag is set, will also write the character into the TTY
 * process output buffer
 * @param c - character to write into the input buffer
 */
void tty_input(char c) {
    if (!active_tty) {
        return;
    }

    ringbuf_write(&active_tty->io_input, c);

    if (active_tty->echo) {
        ringbuf_write(&active_tty->io_output, c);
    }
}

/**
 * Updates the TTY with the given character
 * @param c - character to update on the TTY screen output
 */
void tty_update(char c) {
    if (!active_tty) {
        return;
    }

    struct tty_t *tty = active_tty;
    int index;

    switch (c) {
        case '\t':
            tty->pos_x += 4 - tty->pos_x % 4;
            break;

        case '\b':
        case 127:
            if (tty->pos_x != 0) {
                tty->pos_x--;
                index = (tty->pos_scroll * TTY_WIDTH) +
                        (tty->pos_x + tty->pos_y * TTY_WIDTH);
                tty->buf[index] = ' ';
            } else if (tty->pos_y != 0) {
                tty->pos_y--;
                tty->pos_x = TTY_WIDTH - 1;
                index = (tty->pos_scroll * TTY_WIDTH) +
                        (tty->pos_x + tty->pos_y * TTY_WIDTH);
                tty->buf[index] = ' ';
            }
            break;

        case '\r':
            tty->pos_x = 0;
            break;

        case '\n':
            tty->pos_y++;
            tty->pos_x = 0;
            break;

        default:
            index = (tty->pos_scroll * TTY_WIDTH) +
                    (tty->pos_x + tty->pos_y * TTY_WIDTH);

            if (index >= 0 && index < TTY_BUF_SIZE) {
                tty->buf[index] = c;
            }

            tty->pos_x++;
            break;
    }

    if (tty->pos_x >= TTY_WIDTH) {
        tty->pos_x = 0;
        tty->pos_y++;
    }

    if (tty->pos_y >= TTY_HEIGHT) {
        int x;
        int y;

        for (x = 0; x < TTY_WIDTH; x++) {
            for (y = 1; y < TTY_HEIGHT; y++) {
                tty->buf[TTY_WIDTH * (y - 1) + x] = tty->buf[TTY_WIDTH * y + x];
            }
        }

        for (x = 0; x < TTY_WIDTH; x++) {
            tty->buf[TTY_WIDTH * (TTY_HEIGHT - 1) + x] = ' ';
        }

        tty->pos_y = TTY_HEIGHT - 1;
    }

    tty->refresh = 1;
}

/**
 * Initializes all TTY data structures and memory
 * Selects TTY 0 to be the default
 */
void tty_init(void) {
    kernel_log_info("tty: Initializing TTY driver");

    memset(tty_table, 0, sizeof(tty_table));

    for (int i = 0; i < TTY_MAX; i++) {
        tty_table[i].id = i;
        tty_table[i].color_bg = VGA_COLOR_BLACK;
        tty_table[i].color_fg = VGA_COLOR_LIGHT_GREY;
        tty_table[i].echo = 0;

        memset(tty_table[i].buf, ' ', sizeof(tty_table[i].buf));
        tty_table[i].pos_x = 0;
        tty_table[i].pos_y = 0;
        tty_table[i].pos_scroll = 0;
        tty_table[i].refresh = 1;
    }

    // Select tty 0 to start with
    tty_select(0);

    // Update the screen on a regular interval
    timer_callback_register(tty_refresh, 2, -1);
}