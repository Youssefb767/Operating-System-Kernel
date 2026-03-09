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
    // Set the active tty to point to the entry in the tty table
    // if a new tty is selected, the tty should trigger a refresh
    if (n < 0 || n >= TTY_MAX)
        return;
    if(active_tty != &tty_table[n])
        tty_table[n].refresh = 1;
    active_tty = &tty_table[n];
}

/**
 * Refreshes the tty if needed
 */
void tty_refresh(void) {
    if (!active_tty) {
        kernel_panic("No TTY is selected!");
        return;
    }

    // If the TTY needs to be refreshed, copy the tty buffer
    // to the VGA output.
    // ** hint: use vga_putc_at() since you are setting specific characters
    //          at specific locations
    // Reset the tty's refresh flag so we don't refresh unnecessarily
    if(active_tty->refresh == 1) {
        for(unsigned int y = 0; y < TTY_HEIGHT; y++) {
            for(unsigned int x = 0; x < TTY_HEIGHT; x++) {
                unsigned int i = (active_tty->pos_scroll + y) * TTY_WIDTH + x;
                char c = ' ';
                if (i < TTY_BUF_SIZE) {
                    c = active_tty->buf[idx];
                    if (c == 0) c = ' ';
                }
            vga_putc_at(x, y, active_tty->color_bg, active_tty->color_fg, ch);
        }
        active_tty->refresh = 0;
        }
    }
}

/**
 * Updates the active TTY with the given character
 */
void tty_update(char c) {
    if (!active_tty) {
        return;
    }

    // Since this is a virtual wrapper around the VGA display, treat each
    // input character as you would for the VGA output
    //   Adjust the x/y positions as necessary
    //   Handle scrolling at the bottom

    // Instead of writing to the VGA display directly, you will write
    // to the tty buffer.
    //
    // If the screen should be updated, the refresh flag should be set
    // to trigger the the VGA update via the tty_refresh callback
    
    // Handle scecial characters
    switch (c) {
        case '\b':
            if (active_tty->pos_x != 0) {
                active_tty->pos_x--;
            } else if (active_tty->pos_y != 0) {
                active_tty->pos_y--;
                active_tty->pos_x = TTY_WIDTH-1;
            }

            active_tty->buf[active_tty->pos_x + active_tty->pos_y * TTY_WIDTH] = ' ';
            break;

        case '\t':
            active_tty->pos_x += 4 - active_tty->pos_x % 4;
            break;

        case '\r':
            active_tty->pos_x = 0;
            break;

        case '\n':
            active_tty->pos_x = 0;
            active_tty->pos_y++;
            break;

        default:
            active_tty->buf[active_tty->pos_x + active_tty->pos_y * TTY_WIDTH] = c;
            active_tty->pos_x++;
            break;
    }

    // Handle end of lines
    if (active_tty->pos_x >= TTY_WIDTH) {
        active_tty->pos_x = 0;
        active_tty->pos_y++;
    }

    // Potentially necessary scroll flag
//    if (vga_scroll) {
        // Handle end of rows
        if (active_tty->pos_y >= TTY_HEIGHT) {
            // Scroll the screen up (copy each row to the previous)
            for (unsigned int i = 0; i < TTY_WIDTH * (TTY_HEIGHT - 1); i++) {
                active_tty->buf[i] = active_tty->buf[TTY_WIDTH + i];
            }

            // Clear the last line
            for (unsigned int i = 0; i < TTY_WIDTH; i++) {
                active_tty->buf[i + (TTY_WIDTH * (TTY_HEIGHT-1))] = ' ';
            }

            active_tty->pos_y = TTY_HEIGHT - 1;
        }

//    }

    active_tty->refresh = 1;
}

/**
 * Initializes all TTY data structures and memory
 * Selects TTY 0 to be the default
 */
void tty_init(void) {
    kernel_log_info("tty: Initializing TTY driver");

    // Initialize the tty_table
    for(int i = 0; i < TTY_MAX; i++) {
        tty_table[i].id = i;
        for (int j = 0; j < TTY_BUF_SIZE; j++) {
            tty_table[i].buf[j] = 0;
        }
        tty_table[i].refresh = 1;
        tty_table[i].color_bg = VGA_COLOR_BLACK;
        tty_table[i].color_fg = VGA_COLOR_LIGHT_BLUE;
        tty_table[i].pos_x = 0;
        tty_table[i].pos_y = 0;
        tty_table[i].pos_scroll = 0;
    }

    // Select tty 0 to start with
    tty_select(0);

    // Register a timer callback to update the screen on a regular interval
    timer_callback_register(tty_update, 1, -1);
}


/**
 * Scrolls the TTY up one line into the scrollback buffer
 * If the buffer is at the top, it will not scroll up further
 */
void tty_scroll_up(void){
    if(active_tty->pos_scroll == 0)
        return;
    active_tty->pos_scroll--;
    active_tty->refresh = 1;
}

/**
 * Scrolls the TTY down one line into the scrollback buffer
 * If the buffer is at the end, it will not scroll down further
 */
void tty_scroll_down(void){
    if(active_tty->pos_scroll == TTY_SCROLLBACK)
        return;
    active_tty->pos_scroll++;
    active_tty->refresh = 1;
}

/**
 * Scrolls to the top of the buffer
 */
void tty_scroll_top(void){
    active_tty->pos_scroll = 0;
    active_tty->refresh = 1;
}

/**
 * Scrolls to the bottom of the buffer
 */
void tty_scroll_bottom(void){
    active_tty->pos_scroll = TTY_SCROLLBACK;
    active_tty->refresh = 1;
}