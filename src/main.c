/**
 * CPE/CSC 159 Operating System Pragmatics
 * California State University, Sacramento
 *
 * Operating system entry point
 */

#include <spede/stdbool.h>

#include "interrupts.h"
#include "kernel.h"
#include "keyboard.h"
#include "timer.h"
#include "tty.h"
#include "vga.h"
#include "scheduler.h"
#include "kproc.h"
#include "ksyscall.h"
#include "test.h"
#include "kmutex.h"
#include "ksem.h"
#include "port.h"

int main(void) {
    // Always initialize the kernel
    kernel_init();

    // Initialize interrupts
    interrupts_init();

    // Initialize timers
    timer_init();

    // Initialize the TTY
    tty_init();

    // Initialize the VGA driver
    vga_init();

    // Initialize the keyboard driver
    keyboard_init();

    // Initialize the scheduler
    scheduler_init();

    // Initialize system calls
    ksyscall_init();

    // Initialize kernel semaphores
    ksemaphores_init();

    // Initialize kernel mutexes
    kmutexes_init();

    // Initialize serial ports
    port_init();

    // Initialize processes
    kproc_init();

    // Test initialization
    test_init();

    // Clear the screen
    vga_clear();

    // Enable interrupts
    interrupts_enable();

    // Loop in place forever
    while (1);

    // Should never get here!
    return 0;
}