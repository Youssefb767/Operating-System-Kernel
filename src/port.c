/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 *
 * Serial Port Driver
 */

#include <spede/string.h>
#include <spede/machine/io.h>
#include <spede/machine/rs232.h>

#include "interrupts.h"
#include "kernel.h"
#include "kproc.h"
#include "ksem.h"
#include "port.h"
#include "queue.h"

#define PORT_BAUD_RATE      9600
#define PORT_BAUD_DIVISOR   (115200 / PORT_BAUD_RATE)

#define COM2_IO             0x2F8
#define COM3_IO             0x3E8
#define COM4_IO             0x2E8

// Table of all serial ports
port_t port[PORT_NUM];

/**
 * Initializes the serial port driver.
 */
void port_init(void) {
    kernel_log_info("Initializing Port Serials");

    memset(port, 0, sizeof(port));

    interrupts_irq_register(IRQ_PORT, isr_entry_port, port_irq_handler);
    interrupts_irq_register(IRQ_PORT + 1, isr_entry_port, port_irq_handler);
}

/**
 * Writes one byte from the loopback/write queue to the UART.
 * @param port_num - port index
 */
void PortWriteOne(int port_num) {
    int ch;

    if (port_num < 0 || port_num >= PORT_NUM) {
        kernel_log_error("port: invalid port number %d", port_num);
        return;
    }

    if (port[port_num].owner == NULL) {
        return;
    }

    if (queue_is_empty(&port[port_num].loopback_q) &&
        queue_is_empty(&port[port_num].write_q)) {
        port[port_num].write_ok = 1;
        return;
    }

    if (!queue_is_empty(&port[port_num].loopback_q)) {
        if (queue_out(&port[port_num].loopback_q, &ch) == -1) {
            kernel_log_error("port: failed to dequeue loopback char");
            return;
        }
    } else {
        if (queue_out(&port[port_num].write_q, &ch) == -1) {
            kernel_log_error("port: failed to dequeue write char");
            return;
        }

        ksem_post(port[port_num].write_sid);
    }

    outportb(port[port_num].IO + DATA, ch);

    port[port_num].write_ok = 0;
}

/**
 * Reads one byte from the UART into the read/loopback queues.
 * @param port_num - port index
 */
void PortReadOne(int port_num) {
    int raw;
    int ch;

    if (port_num < 0 || port_num >= PORT_NUM) {
        kernel_log_error("port: invalid port number %d", port_num);
        return;
    }

    if (port[port_num].owner == NULL) {
        return;
    }

    raw = inportb(port[port_num].IO + DATA);
    ch = raw & 0x7F;

    if (queue_is_full(&port[port_num].read_q)) {
        kernel_log_error("port: read queue full on port %d", port_num);
        return;
    }

    if (queue_in(&port[port_num].read_q, ch) == -1) {
        kernel_log_error("port: failed to enqueue read char on port %d", port_num);
        return;
    }

    if (ch != '\b' && ch != 0x7F) {
    if (!queue_is_full(&port[port_num].loopback_q)) {
        queue_in(&port[port_num].loopback_q, ch);
    } else {
        kernel_log_error("port: loopback queue full on port %d", port_num);
    }

    if (ch == '\r') {
        if (!queue_is_full(&port[port_num].loopback_q)) {
            queue_in(&port[port_num].loopback_q, '\n');
        }
    }
}

    if (ch == '\r') {
        if (!queue_is_full(&port[port_num].loopback_q)) {
            queue_in(&port[port_num].loopback_q, '\n');
        }
    }

    ksem_post(port[port_num].read_sid);

    if (port[port_num].write_ok != 0) {
        PortWriteOne(port_num);
    }
}

/**
 * IRQ handler for serial port interrupts.
 */
void port_irq_handler(void) {
    int iir;

    for (int i = 0; i < PORT_NUM; i++) {
        if (port[i].owner == NULL) {
            continue;
        }

        iir = inportb(port[i].IO + IIR);

        if (iir == IIR_RXRDY) {
            PortReadOne(i);
        } else if (iir == IIR_TXRDY) {
            PortWriteOne(i);
        }

        if (port[i].write_ok != 0) {
            PortWriteOne(i);
        }
    }

    /*
     * Do not manually send PIC EOI here.
     * This project's interrupts_irq_handler() already calls pic_irq_dismiss().
     */
}

/**
 * Allocates a serial port for the active process.
 * @return port index on success, -1 on error
 */
int PortAllocHandler(void) {
    static int port_io[PORT_NUM] = {
        COM2_IO,
        COM3_IO,
        COM4_IO
    };

    int p = -1;
    int divisor = PORT_BAUD_DIVISOR;

    if (!active_proc) {
        kernel_log_error("port: no active process");
        return -1;
    }

    for (int i = 0; i < PORT_NUM; i++) {
        if (port[i].owner == NULL) {
            p = i;
            break;
        }
    }

    if (p < 0) {
        kernel_panic("port: no free serial ports");
        return -1;
    }

    memset(&port[p], 0, sizeof(port_t));

    port[p].owner = active_proc;
    port[p].IO = port_io[p];
    port[p].write_sid = -1;
    port[p].read_sid = -1;
    port[p].write_ok = 1;

    if (queue_init(&port[p].write_q) == -1) {
        kernel_log_error("port: failed to initialize write queue");
        port[p].owner = NULL;
        return -1;
    }

    if (queue_init(&port[p].read_q) == -1) {
        kernel_log_error("port: failed to initialize read queue");
        port[p].owner = NULL;
        return -1;
    }

    if (queue_init(&port[p].loopback_q) == -1) {
        kernel_log_error("port: failed to initialize loopback queue");
        port[p].owner = NULL;
        return -1;
    }

    // 1. Set DLAB to access divisor registers
    outportb(port[p].IO + CFCR, CFCR_DLAB);

    // 2. Write 16-bit baud divisor
    outportb(port[p].IO + BAUDLO, divisor & 0xFF);
    outportb(port[p].IO + BAUDHI, (divisor >> 8) & 0xFF);

    // 3. Set 7 data bits, even parity, 1 stop bit; clear DLAB
    outportb(port[p].IO + CFCR, CFCR_PEVEN | CFCR_PENAB | CFCR_7BITS);

    // 4. Disable interrupts briefly
    outportb(port[p].IO + IER, 0x0);

    // 5. Enable DTR, RTS, and master interrupt enable
    outportb(port[p].IO + MCR, MCR_DTR | MCR_RTS | MCR_IENABLE);

    // 6. Short hardware settle delay
    for (int i = 0; i <= 0x63; i++) {
        asm volatile("inb $0x80");
    }

    // 7. Enable RX and TX interrupts
    outportb(port[p].IO + IER, IER_ERXRDY | IER_ETXRDY);

    kernel_log_info("port: allocated port %d IO=0x%03x to pid %d",
                    p,
                    port[p].IO,
                    active_proc->pid);

    return p;
}

/**
 * Kernel-side handler for writing one byte to a serial port.
 * @param one - byte to write
 * @param port_num - port index
 */
void PortWriteHandler(char one, int port_num) {
    if (port_num < 0 || port_num >= PORT_NUM) {
        kernel_log_error("port: invalid port number %d", port_num);
        return;
    }

    if (port[port_num].owner == NULL) {
        kernel_log_error("port: write to unallocated port %d", port_num);
        return;
    }

    if (queue_is_full(&port[port_num].write_q)) {
        kernel_log_error("port: write queue full on port %d", port_num);
        return;
    }

    if (queue_in(&port[port_num].write_q, one) == -1) {
        kernel_log_error("port: failed to enqueue write char");
        return;
    }

    if (port[port_num].write_ok != 0) {
        PortWriteOne(port_num);
    }
}

/**
 * Kernel-side handler for reading one byte from a serial port.
 * @param port_num - port index
 * @return received byte or -1 on error
 */
int PortReadHandler(int port_num) {
    int ch;

    if (port_num < 0 || port_num >= PORT_NUM) {
        kernel_log_error("port: invalid port number %d", port_num);
        return -1;
    }

    if (port[port_num].owner == NULL) {
        kernel_log_error("port: read from unallocated port %d", port_num);
        return -1;
    }

    if (queue_is_empty(&port[port_num].read_q)) {
        kernel_log_error("port: read queue empty on port %d", port_num);
        return -1;
    }

    if (queue_out(&port[port_num].read_q, &ch) == -1) {
        kernel_log_error("port: failed to dequeue read char");
        return -1;
    }

    return ch;
}