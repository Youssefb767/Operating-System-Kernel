#include <spede/stdarg.h>
#include <spede/stdio.h>

#include "bit.h"
#include "io.h"
#include "kernel.h"
#include "vga.h"



/**
 * Forward Declarations
 */
void vga_cursor_update(void);

/**
 * Global variables in this file scope
 */
static int vga_row = 0;
static int vga_col = 0;
static int vga_bg  = VGA_COLOR_BLACK;
static int vga_fg  = VGA_COLOR_LIGHT_GREY;
static bool vga_cursor_is_enabled = false;

/**
 * Initializes the VGA driver and configuration
 *  - Defaults variables
 *  - Clears the screen
 */
void vga_init(void) {
    kernel_log_info("Initializing VGA driver");
    
    vga_row = 0;
    vga_col = 0;
    vga_bg  = VGA_COLOR_BLACK;
    vga_fg  = VGA_COLOR_LIGHT_GREY;
    vga_cursor_is_enabled = false;   // or true if you want cursor by default

    vga_clear();// Clear the screen

    // Optional: enable cursor now
    // vga_cursor_enable();
}

/**
 * Clears the VGA output and sets the background and foreground colors
 */
void vga_clear(void) {
    // Set the cursor position to the top-left corner (0, 0)
    // Clear entire screen using current bg/fg
    unsigned short *vga_buf = VGA_BASE;
    unsigned short cell = VGA_CHAR(vga_bg, vga_fg, ' ');

    for (int r = 0; r < VGA_HEIGHT; r++) {
        for (int c = 0; c < VGA_WIDTH; c++) {
            vga_buf[r * VGA_WIDTH + c] = cell;
        }
    }

    vga_row = 0;
    vga_col = 0;
    vga_cursor_update();
}

/**
 * Clears the background color for all characters and sets to the
 * specified value
 *
 * @param bg background color value
 */
void vga_clear_bg(int bg) {
    // Update state + rewrite background bits for all cells
    if (bg < 0) bg = 0;
    if (bg > 0xF) bg = 0xF;
    vga_bg = bg;

    unsigned short *vga_buf = VGA_BASE;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        unsigned char ch = (unsigned char)(vga_buf[i] & 0x00FF);
        unsigned char attr = (unsigned char)((vga_buf[i] & 0xFF00) >> 8);
        unsigned char old_fg = attr & 0x0F;
        vga_buf[i] = VGA_CHAR(vga_bg, old_fg, ch);
    }
}

/**
 * Clears the foreground color for all characters and sets to the
 * specified value
 *
 * @param fg foreground color value
 */
void vga_clear_fg(int fg) {
    // Iterate through all VGA memory and set only the foreground color bits.
    if (fg < 0) fg = 0;
    if (fg > 0xF) fg = 0xF;
    vga_fg = fg;

    unsigned short *vga_buf = VGA_BASE;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        unsigned char ch = (unsigned char)(vga_buf[i] & 0x00FF);
        unsigned char attr = (unsigned char)((vga_buf[i] & 0xFF00) >> 8);
        unsigned char old_bg = (attr >> 4) & 0x0F;
        vga_buf[i] = VGA_CHAR(old_bg, vga_fg, ch);
    }
}

/**
 * Enables the VGA text mode cursor
 */
void vga_cursor_enable(void) {
    unsigned char cur;
    // All operations will consist of writing to the address port which
    // register should be set, followed by writing to the data port the value
    // to set for the specified register

    // The cursor will be drawn between the scanlines defined
    // in the following registers:
    //   0x0A Cursor Start Register
    //   0x0B Cursor End Register

    // Bit 5 in the cursor start register (0x0A) will enable or disable the cursor:
    //   0 - Cursor enabled
    //   1 - Cursor disabled

    // The cursor will be displayed as a series of horizontal lines that create a
    // "block" or rectangular symbol. The position/size is determined by the "starting"
    // scanline and the "ending" scanline. Scanlines range from 0x0 to 0xF.

    // In both the cursor start register and cursor end register, the scanline values
    // are specified in bits 0-4

    // To ensure that we do not change bits we are not intending,
    // read the current register state and mask off the bits we
    // want to save

    // Set the cursor starting scanline (register 0x0A, cursor start register)
    // Ensure that bit 5 is not set so the cursor will be enabled
    outportb(VGA_PORT_ADDR, 0x0A);
    cur = inportb(VGA_PORT_DATA);
    cur = (cur & 0xC0) | 0x02;
    outportb(VGA_PORT_DATA, cur);

    // Set the cursor ending scanline (register 0x0B, cursor end register)    
    outportb(VGA_PORT_ADDR, 0x0B);
    cur = inportb(VGA_PORT_DATA);
    cur = (cur & 0xE0) | 0x0C;
    outportb(VGA_PORT_DATA, cur);

    // Since we may need to update the vga text mode cursor position in
    // the future, ensure that we track (via software) if the cursor is
    // enabled or disabled
    vga_cursor_is_enabled = true;

    // Update the cursor location once it is enabled
    vga_cursor_update();
}

/**
 * Disables the VGA text mode cursor
 */
void vga_cursor_disable(void) {
    unsigned char cur;
    // All operations will consist of writing to the address port which
    // register should be set, followed by writing to the data port the value
    // to set for the specified register

    // The cursor will be drawn between the scanlines defined
    // in the following registers:
    //   0x0A Cursor Start Register
    //   0x0B Cursor End Register

    // Bit 5 in the cursor start register (0x0A) will enable or disable the cursor:
    //   0 - Cursor enabled
    //   1 - Cursor disabled

    // Since we are disabling the cursor, we can simply set the bit of interest
    // as we won't care what the current cursor scanline start/stop values are
    outportb(VGA_PORT_ADDR, 0x0A);
    cur = inportb(VGA_PORT_DATA);
    cur = (cur | 0x20);
    outportb(VGA_PORT_DATA, cur);

    // Since we may need to update the vga text mode cursor position in
    // the future, ensure that we track (via software) if the cursor is
    // enabled or disabled
    vga_cursor_is_enabled = false;
}

/**
 * Indicates if the VGA text mode cursor is enabled or disabled
 */
bool vga_cursor_enabled(void) {
    return vga_cursor_is_enabled;
}

/**
 * Sets the vga text mode cursor position to the current VGA row/column
 * position if the cursor is enabled
 */
void vga_cursor_update(void) {
    // All operations will consist of writing to the address port which
    // register should be set, followed by writing to the data port the value
    // to set for the specified register

    // The cursor position is represented as an unsigned short (2-bytes). As
    // VGA register values are single-byte, the position representation is
    // split between two registers:
    //   0x0F Cursor Location High Register
    //   0x0E Cursor Location Low Register

    // The Cursor Location High Register is the least significant byte
    // The Cursor Location Low Register is the most significant byte

    if(!vga_cursor_is_enabled) return;

    // If the cursor is enabled:
    // Calculate the cursor position as an offset into
    // memory (unsigned short value)
    unsigned short pos = (unsigned short)(vga_row * VGA_WIDTH + vga_col);

    // Set the VGA Cursor Location High Register (0x0F)
    //   Should be the least significant byte (0x??<00>)
    outportb(VGA_PORT_ADDR, 0x0F);
    outportb(VGA_PORT_DATA, (unsigned char)(pos & 0xFF));
    // Set the VGA Cursor Location Low Register (0x0E)
    //   Should be the most significant byte (0x<00>??)
    outportb(VGA_PORT_ADDR, 0x0E);
    outportb(VGA_PORT_DATA, (unsigned char)((pos >> 8) & 0xFF));
}

/**
 * Sets the current row/column position
 *
 * @param row position (0 to VGA_HEIGHT-1)
 * @param col position (0 to VGA_WIDTH-1)
 * @notes If the input parameters exceed the valid range, the position
 *        will be set to the range boundary (min or max)
 */
void vga_set_rowcol(int row, int col) {
    // Update the text mode cursor (if enabled)
    if (row < 0) row = 0;
    if (row >= VGA_HEIGHT) row = VGA_HEIGHT - 1;
    if (col < 0) col = 0;
    if (col >= VGA_WIDTH) col = VGA_WIDTH - 1;

    vga_row = row;
    vga_col = col;
    vga_cursor_update();
}

/**
 * Gets the current row position
 * @return integer value of the row (between 0 and VGA_HEIGHT-1)
 */
int vga_get_row(void) {
    return vga_row;
}

/**
 * Gets the current column position
 * @return integer value of the column (between 0 and VGA_WIDTH-1)
 */
int vga_get_col(void) {
    return vga_col;
}

/**
 * Sets the background color.
 *
 * Does not modify any existing background colors, only sets it for
 * new operations.
 *
 * @param bg - background color
 */
void vga_set_bg(int bg) {
    if (bg < 0) bg = 0;
    if (bg > 0xF) bg = 0xF;
    vga_bg = bg;
}

/**
 * Gets the current background color
 * @return background color value
 */
int vga_get_bg(void) {
    return vga_bg;
}

/**
 * Sets the foreground/text color.
 *
 * Does not modify any existing foreground colors, only sets it for
 * new operations.
 *
 * @param color - background color
 */
void vga_set_fg(int fg) {
    if (fg < 0) fg = 0;
    if (fg > 0xF) fg = 0xF;
    vga_fg = fg;
}

/**
 * Gets the current foreground color
 * @return foreground color value
 */
int vga_get_fg(void) {
    return vga_fg;
}

/**
 * Prints a character on the screen without modifying the cursor or other attributes
 *
 * @param c - Character to print
 */
void vga_setc(unsigned char c) {
    unsigned short *vga_buf = VGA_BASE;
    int idx = vga_row * VGA_WIDTH + vga_col;
    vga_buf[idx] = VGA_CHAR(vga_bg, vga_fg, c);
}

/**
 * Prints a character on the screen at the current cursor (row/column) position
 *
 * When a character is printed, will do the following:
 *  - Update the row and column positions
 *  - If needed, will wrap from the end of the current line to the
 *    start of the next line
 *  - If the last line is reached, the cursor position will reset to the top-left (0, 0) position
 *  - Special characters are handled as such:
 *    - tab character (\t) prints 'tab_stop' spaces
 *    - backspace (\b) character moves the character back one position,
 *      prints a space, and then moves back one position again
 *    - new-line (\n) should move the cursor to the beginning of the next row
 *    - carriage return (\r) should move the cursor to the beginning of the current row
 *
 * @param c - character to print
 */
void vga_putc(unsigned char c) {
    // Handle scecial characters
    if (c == '\n') {
        // New line
        vga_col = 0;
        vga_row++;
    } else if (c == '\r') {
        // Carriage return
        vga_col = 0;
    } else if (c == '\t') {
        // Tab: 4 spaces
        for (int i = 0; i < 4; i++) {
            vga_putc(' ');
        }
        return;
    } else if (c == '\b') {
        // Backspace
        if (vga_col > 0) {
            vga_col--;
        } else {
            if (vga_row > 0) {
                vga_row--;
                vga_col = VGA_WIDTH - 1;
            } else {
            // Cursor is already top left
            return;
            }
        }
        // Erase character at new position
        vga_setc(' ');
    } else {
        vga_setc(c);
        vga_col++;
    }

    // Handle end of lines
    if (vga_col >= VGA_WIDTH) {
        vga_col = 0;
        vga_row++;
    }

    // Wrap-around to the top/left corner
    if (vga_row >= VGA_HEIGHT) {
        vga_row = 0;
        vga_col = 0;
    }

    // Update the text mode cursor, if enabled
    if (vga_cursor_is_enabled) vga_cursor_update();
}

/**
 * Prints a string on the screen at the current cursor (row/column) position
 *
 * @param s - string to print
 */
void vga_puts(char *s) {
    if (s == NULL) return;

    for (int i = 0; s[i] != '\0'; i++) {
        vga_putc((unsigned char)s[i]);
    }
}

/**
 * Prints a character on the screen at the specified row/column position and
 * with the specified background/foreground colors
 *
 * Does not modify the current row or column position
 * Does not modify the current background or foreground colors
 *
 * @param row the row position (0 to VGA_HEIGHT-1)
 * @param col the column position (0 to VGA_WIDTH-1)
 * @param bg background color
 * @param fg foreground color
 * @param c character to print
 */
void vga_putc_at(int row, int col, int bg, int fg, unsigned char c) {
    if (row < 0 || row >= VGA_HEIGHT) return;
    if (col < 0 || col >= VGA_WIDTH) return;

    if (bg < 0) bg = 0;
    if (bg > 0xF) bg = 0xF;
    if (fg < 0) fg = 0;
    if (fg > 0xF) fg = 0xF;

    unsigned short *vga_buf = VGA_BASE;
    vga_buf[row * VGA_WIDTH + col] = VGA_CHAR(bg, fg, c);
}

/**
 * Prints a string on the screen at the specified row/column position and
 * with the specified background/foreground colors
 *
 * Does not modify the current row or column position
 * Does not modify the current background or foreground colors
 *
 * @param row the row position (0 to VGA_HEIGHT-1)
 * @param col the column position (0 to VGA_WIDTH-1)
 * @param bg background color
 * @param fg foreground color
 * @param s string to print
 */
void vga_puts_at(int row, int col, int bg, int fg, char *s) {
    if (!s) return;
    if (row < 0 || row >= VGA_HEIGHT) return;
    if (col < 0) col = 0;
    if (col >= VGA_WIDTH) return;

    for (int i = 0; s[i] != '\0' && (col + i) < VGA_WIDTH; i++) {
        vga_putc_at(row, col + i, bg, fg, (unsigned char)s[i]);
    }
}

