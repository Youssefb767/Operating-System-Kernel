/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * 
 *
 * TTY Definitions
 */

#include <spede/string.h>

#include "kernel.h"
#include "timer.h"
#include "tty.h"
#include "vga.h"
#include "ringbuf.h"

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
 * Returns a pointer to the specified TTY table entry
 * @param tty - TTY id
 * @return pointer to TTY table entry
 */
struct tty_t *tty_get(int tty) {
    if (tty < 0 || tty >= TTY_MAX) {
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

    // Handle new I/O (characters in the output buffer)
    while (!ringbuf_is_empty(&tty->io_output)) {
        // Read next character from ring buffer
        if (ringbuf_read(&tty->io_output, &c) == 0) {
            // Send next characture to the tty screen buffer
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

            vga_putc_at(x++, y, tty->color_bg, tty->color_fg, tty->buf[tty->pos_scroll*TTY_WIDTH + i]);
        }

        // The screen has been refreshed, so clea the refresh flag
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

    // Write into the input ring buffer
    ringbuf_write(&active_tty->io_input, c);

    // If echo enabled, also send output to ringbuf
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

//    kernel_log_debug("tty[%d]: input char=%c", tty->id, c);
//    kernel_log_debug("  before scroll=%d, x=%d, y=%d", tty->pos_scroll, tty->pos_x, tty->pos_y);

    switch (c) {
        case '\t':
            tty->pos_x += 4 - tty->pos_x % 4;
            break;

        case '\b':
            if (tty->pos_x != 0) {
                tty->pos_x--;
            } else if (tty->pos_y != 0) {
                tty->pos_y--;
                tty->pos_x = TTY_WIDTH - 1;
            }
            tty->buf[(tty->pos_scroll * TTY_WIDTH) + (tty->pos_x + tty->pos_y * TTY_WIDTH)] = ' ';
            break;

        case '\r':
            tty->pos_x = 0;
            break;

        case '\n':
            tty->pos_y++;
            tty->pos_x = 0;
            break;

        default:
            tty->buf[(tty->pos_scroll * TTY_WIDTH) + (tty->pos_x + tty->pos_y * TTY_WIDTH)] = c;
            tty->pos_x++;
            break;
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
            tty->buf[TTY_WIDTH * (y - 1) + x] = ' ';
        }

        tty->pos_y = TTY_HEIGHT - 1;
    }

//    kernel_log_debug("  after: scroll=%d, x=%d, y=%d", tty->pos_scroll, tty->pos_x, tty->pos_y);
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
        tty_table[i].id=i;
        tty_table[i].color_bg = VGA_COLOR_BLACK;
        tty_table[i].color_fg = VGA_COLOR_LIGHT_GREY;
        tty_table[i].echo = 0;
        ringbuf_init(&tty_table[i].io_input);
        ringbuf_init(&tty_table[i].io_output);
    }

    // Select tty 0 to start with
    tty_select(0);

    // Update the screen on a regular interval (50 times per second right now)
    timer_callback_register(tty_refresh, 2, -1);
}

