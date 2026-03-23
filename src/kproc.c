/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 *
 * Kernel Process Handling
 */

#include <spede/stdio.h>
#include <spede/string.h>
#include <spede/machine/proc_reg.h>

#include "trapframe.h"
#include "kproc.h"
#include "kernel.h"
#include "scheduler.h"
#include "timer.h"
#include "queue.h"
#include "vga.h"

// Next available process id to be assigned
int next_pid;

// Process table allocator
queue_t proc_allocator;

// Process table
proc_t proc_table[PROC_MAX];

// Process stacks
unsigned char proc_stack[PROC_MAX][PROC_STACK_SIZE];

/**
 * Looks up a process in the process table via the process id
 * @param pid - process id
 * @return pointer to the process entry, NULL or error or if not found
 */
proc_t *pid_to_proc(int pid) {
    int i;

    if (pid < 0) {
        return NULL;
    }

    for (i = 0; i < PROC_MAX; i++) {
        if (proc_table[i].state != NONE && proc_table[i].pid == pid) {
            return &proc_table[i];
        }
    }

    return NULL;
}

/**
 * Translates a process pointer to the entry index into the process table
 * @param proc - pointer to a process entry
 * @return the index into the process table, -1 on error
 */
int proc_to_entry(proc_t *proc) {
    int entry;

    if (proc == NULL) {
        return -1;
    }

    if (proc < &proc_table[0] || proc >= &proc_table[PROC_MAX]) {
        return -1;
    }

    entry = (int)(proc - &proc_table[0]);

    if (proc_table[entry].state == NONE) {
        return -1;
    }

    return entry;
}

/**
 * Returns a pointer to the given process entry
 */
proc_t *entry_to_proc(int entry) {
    // For the given entry number, return a pointer to the process table entry
    // Ensure that the process control block actually refers to a valid process
    return NULL;
}

/**
 * Creates a new process
 * @param proc_ptr - address of process to execute
 * @param proc_name - "friendly" process name
 * @param proc_type - process type (kernel or user)
 * @return process id of the created process, -1 on error
 */
int kproc_create(void *proc_ptr, char *proc_name, proc_type_t proc_type) {
    proc_t *proc = NULL;

    // Allocate an entry in the process table via the process allocator
    int entry;
    if (!proc_ptr || !proc_name || queue_out(&proc_allocator, &entry) != 0) {
        return -1;
    }
    proc = &proc_table[entry];

    // Initialize the process control block
    memset(proc, 0, sizeof(proc_t));
    memset(proc_stack[entry], 0, PROC_STACK_SIZE);

    // Initialize the process stack via proc_stack
    proc->stack = proc_stack[entry];

    // Initialize the trapframe pointer at the bottom of the stack
    proc->trapframe = (trapframe_t *)(&proc->stack[PROC_STACK_SIZE - sizeof(trapframe_t)]);

    // Set each of the process control block structure members to the initial starting values
    proc->pid = next_pid;
    next_pid++;

    proc->state = IDLE;
    proc->type = proc_type;
    proc->run_time = 0;
    proc->cpu_time = 0;
    proc->start_time = timer_get_ticks();

    // Copy the passed-in name to the name buffer in the process control block
    strncpy(proc->name, proc_name, PROC_NAME_LEN - 1);
    proc->name[PROC_NAME_LEN - 1] = '\0';

    // Set the instruction pointer in the trapframe
    proc->trapframe->eip = (unsigned int)proc_ptr;

    // Set INTR flag
    proc->trapframe->eflags = EF_DEFAULT_VALUE | EF_INTR;

    // Set each segment in the trapframe
    proc->trapframe->cs = get_cs();
    proc->trapframe->ds = get_ds();
    proc->trapframe->es = get_es();
    proc->trapframe->fs = get_fs();
    proc->trapframe->gs = get_gs();

    // Add the process to the scheduler
    scheduler_add(proc);

    kernel_log_info("Created process %s (%d) entry=%d", proc->name, proc->pid, entry);

    return proc->pid;
}

/**
 * Destroys a process
 * If the process is currently scheduled it must be unscheduled
 * @param proc - process control block
 * @return 0 on success, -1 on error
 */
int kproc_destroy(proc_t *proc) {
    int entry;

    if (!proc || proc->pid == 0) {
        return -1;
    }

    entry = proc_to_entry(proc);
    if (entry < 0) {
        return -1;
    }

    // Remove the process from the scheduler
    scheduler_remove(proc);

    // Clear/Reset all process data (process control block, stack, etc) related to the process
    if (active_proc == proc) {
        active_proc = NULL;
    }

    memset(proc, 0, sizeof(proc_t));
    memset(proc_stack[entry], 0, PROC_STACK_SIZE);

    proc->pid = -1;
    proc->state = NONE;
    proc->type = PROC_TYPE_NONE;
    proc->stack = NULL;
    proc->trapframe = NULL;

    // Add the process entry/index value back into the process allocator
    return queue_in(&proc_allocator, entry);
}

/**
 * Idle Process
 */
void kproc_idle(void) {
    while (1) {
        // Ensure interrupts are enabled
        asm("sti");

        // Halt the CPU
        asm("hlt");
    }
}

/**
 * Test process
 */
void kproc_test(void) {
    // Loop forever
    while (1);
}

/**
 * Initializes all process related data structures
 * Creates the first process (kernel_idle)
 * Registers the callback to display the process table/status
 */
void kproc_init(void) {
    int i;

    kernel_log_info("Initializing process management");

    // Initialize all data structures and variables
    //   - process table
    //   - process allocator
    //   - process stack
    next_pid = 0;

    memset(proc_table, 0, sizeof(proc_table));
    memset(proc_stack, 0, sizeof(proc_stack));

    queue_init(&proc_allocator);

    for (i = 0; i < PROC_MAX; i++) {
        proc_table[i].pid = -1;
        proc_table[i].state = NONE;
        proc_table[i].type = PROC_TYPE_NONE;
        proc_table[i].name[0] = '\0';
        proc_table[i].start_time = 0;
        proc_table[i].run_time = 0;
        proc_table[i].cpu_time = 0;
        proc_table[i].stack = NULL;
        proc_table[i].trapframe = NULL;

        if (queue_in(&proc_allocator, i) != 0) {
            kernel_log_warn("kproc: unable to queue allocator entry %d", i);
        }
    }

    // Create the idle process (kproc_idle) as a kernel process
    if (kproc_create(kproc_idle, "idle", PROC_TYPE_KERNEL) < 0) {
        kernel_panic("kproc: unable to create idle process");
    }
}