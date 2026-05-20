/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 *
 * Serial Port Driver
 */
#ifndef PORT_H
#define PORT_H

#include "kproc.h"
#include "queue.h"

#ifndef PORT_NUM
#define PORT_NUM 3
#endif

#ifndef PORT_READ_BUF_SIZE
#define PORT_READ_BUF_SIZE 128
#endif

typedef struct port_t {
    proc_t *owner;          // Process that allocated this port
    int IO;                 // UART I/O base address
    int write_sid;          // Semaphore for free TX queue slots
    int read_sid;           // Semaphore for received bytes
    int write_ok;           // UART TX idle flag
    queue_t write_q;        // Bytes waiting to transmit
    queue_t read_q;         // Bytes received from UART
    queue_t loopback_q;     // Bytes waiting to echo back
} port_t;

extern port_t port[PORT_NUM];

/**
 * Initializes the serial port driver.
 */
void port_init(void);

/**
 * Writes one byte from the loopback/write queue to the UART.
 * @param port_num - port index
 */
void PortWriteOne(int port_num);

/**
 * Reads one byte from the UART into the read/loopback queues.
 * @param port_num - port index
 */
void PortReadOne(int port_num);

/**
 * IRQ handler for serial port interrupts.
 */
void port_irq_handler(void);

/**
 * Allocates a serial port for the active process.
 * @return port index on success, -1 on error
 */
int PortAllocHandler(void);

/**
 * Kernel-side handler for writing one byte to a serial port.
 * @param one - byte to write
 * @param port_num - port index
 */
void PortWriteHandler(char one, int port_num);

/**
 * Kernel-side handler for reading one byte from a serial port.
 * @param port_num - port index
 * @return received byte or -1 on error
 */
int PortReadHandler(int port_num);

#endif