/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 *
 * Keyboard Functions
 */
#include "io.h"
#include "kernel.h"
#include "keyboard.h"
#include <stdbool.h>

/* keyboard state */
static bool kb_shift = false;
static bool kb_caps  = false;

/**
 * Initializes keyboard data structures and variables
 */
void keyboard_init() {
    kernel_log_info("Initializing keyboard driver");
}

/**
 * Scans for keyboard input and returns the raw character data
 * @return raw character data from the keyboard
 */
unsigned int keyboard_scan(void) {
    unsigned int c = KEY_NULL;
    return c;
}

/**
 * Polls for a keyboard character to be entered.
 *
 * If a keyboard character data is present, will scan and return
 * the decoded keyboard output.
 *
 * @return decoded character or KEY_NULL (0) for any character
 *         that cannot be decoded
 */
unsigned int keyboard_poll(void) {
    unsigned int c = KEY_NULL;
    return c;
}

/**
 * Blocks until a keyboard character has been entered
 * @return decoded character entered by the keyboard or KEY_NULL
 *         for any character that cannot be decoded
 */
unsigned int keyboard_getc(void) {
    unsigned int c = KEY_NULL;
    while ((c = keyboard_poll()) == KEY_NULL);
    return c;
}

/**
 * Processes raw keyboard input and decodes it.
 *
 * Should keep track of the keyboard status for the following keys:
 *   SHIFT, CTRL, ALT, CAPS, NUMLOCK
 *
 * For all other characters, they should be decoded/mapped to ASCII
 * or ASCII-friendly characters.
 *
 * For any character that cannot be mapped, KEY_NULL should be returned.
 *
 */
unsigned int keyboard_decode(unsigned int scan_code) {
    if (scan_code == KEY_NULL) return KEY_NULL;
    if (scan_code & 0x80) return KEY_NULL; // only decode on release
    unsigned int key_code = scan_code & 0x7F;

    // Special keys
    switch (key_code) {
        case 0x01: return KEY_ESCAPE; // Escape
        case 0x1C: return '\n';     // Enter
        case 0x0E: return '\b';     // Backspace
        case 0x0F: return '\t';     // Tab
        case 0x39: return ' ';      // Space
        default: break;
    }

    unsigned char c = 0;

    // Letters
    switch(key_code) {
        case 0x1E: c = 'a'; break;
        case 0x30: c = 'b'; break;
        case 0x2E: c = 'c'; break;
        case 0x20: c = 'd'; break;
        case 0x12: c = 'e'; break;
        case 0x21: c = 'f'; break;
        case 0x22: c = 'g'; break;
        case 0x23: c = 'h'; break;
        case 0x17: c = 'i'; break;
        case 0x24: c = 'j'; break;
        case 0x25: c = 'k'; break;
        case 0x26: c = 'l'; break;
        case 0x32: c = 'm'; break;
        case 0x31: c = 'n'; break;
        case 0x18: c = 'o'; break;
        case 0x19: c = 'p'; break;
        case 0x10: c = 'q'; break;
        case 0x13: c = 'r'; break;
        case 0x1F: c = 's'; break;
        case 0x14: c = 't'; break;
        case 0x16: c = 'u'; break;
        case 0x2F: c = 'v'; break;
        case 0x11: c = 'w'; break;
        case 0x2D: c = 'x'; break;
        case 0x15: c = 'y'; break;
        case 0x2C: c = 'z'; break;
        default: break;
    }

    if (c) {
        if (kb_shift ^ kb_caps) 
            c = (unsigned char)(c - 'a' + 'A');
        return c;
    }

    // Numbers
    c = 0;
    switch (key_code) {
        case 0x02: c = '1'; break;
        case 0x03: c = '2'; break;
        case 0x04: c = '3'; break;
        case 0x05: c = '4'; break;
        case 0x06: c = '5'; break;
        case 0x07: c = '6'; break;
        case 0x08: c = '7'; break;
        case 0x09: c = '8'; break;
        case 0x0A: c = '9'; break;
        case 0x0B: c = '0'; break;
        default: break;
    }

    if (c) {
        if (!kb_shift) return c;

        switch(c) {
            case '1': return '!';
            case '2': return '@';
            case '3': return '#';
            case '4': return '$';
            case '5': return '%';
            case '6': return '^';
            case '7': return '&';
            case '8': return '*';
            case '9': return '(';
            case '0': return ')';
        }
        return c;
    }

    // Basic punctuation
    c = 0;
    switch (key_code) {
        case 0x0C: c = '-'; break;
        case 0x0D: c = '='; break;
        case 0x1A: c = '['; break;
        case 0x1B: c = ']'; break;
        case 0x2B: c = '\\'; break;
        case 0x27: c = ';'; break;
        case 0x28: c = '\''; break;
        case 0x29: c = '`'; break;
        case 0x33: c = ','; break;
        case 0x34: c = '.'; break;
        case 0x35: c = '/'; break;
        default: return KEY_NULL;
    }

    if (!kb_shift) return c;

    switch (c) {
        case '-':  return '_';
        case '=':  return '+';
        case '[':  return '{';
        case ']':  return '}';
        case '\\': return '|';
        case ';':  return ':';
        case '\'': return '"';
        case '`':  return '~';
        case ',':  return '<';
        case '.':  return '>';
        case '/':  return '?';
    }

    return c;
}
