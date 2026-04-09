/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 *
 * Kernel Process Handling
 */

#include <spede/string.h>
#include <spede/stdio.h>
#include <spede/time.h>
#include <spede/machine/proc_reg.h>

#include "kernel.h"
#include "kproc.h"
#include "scheduler.h"
#include "timer.h"

#include "queue.h"

// Process Queues
queue_t run_queue;      // Run queue -> processes that will be scheduled to run

// From kproc.c
extern proc_t *active_proc;

/**
 * Scheduler timer callback
 */
void scheduler_timer(void) {
    // Update the active process' run time and CPU time
    if (!active_proc)
        return;

    if (active_proc->state != NONE) {
        active_proc->run_time++;
        active_proc->cpu_time++;
    }
}

/**
 * Executes the scheduler
 * Should ensure that `active_proc` is set to a valid process entry
 */
void scheduler_run(void) {
    int pid;

    // Ensure that processes not in the active state aren't still scheduled
<<<<<<< syscall
    if (active_proc && active_proc->state != ACTIVE) {
        active_proc = NULL;
    }
=======
    if (active_proc && active_proc->state == NONE)
        active_proc = NULL;
>>>>>>> main

    // Check if we have an active process
    if (active_proc) {
        // Check if the current process has exceeded it's time slice
        if (active_proc->cpu_time >= SCHEDULER_TIMESLICE) {
            // Reset the active time
            active_proc->cpu_time = 0;

            // If the process is not the idle task, add it back to the scheduler
            if (active_proc->pid != 0)
                scheduler_add(active_proc);
            // Otherwise, simply set the state to IDLE
            else
                active_proc->state = IDLE;

<<<<<<< syscall
            if (active_proc->pid != 0) {
                // Add the process to the scheuler
                scheduler_add(active_proc);
            } else {
                active_proc->state = IDLE;
            }

            // Unschedule the current process
            kernel_log_trace("Unscheduling process pid=%d, name=%s", active_proc->pid, active_proc->name);
            active_proc = NULL;
        }
    }

    // Check if we have a process scheduled or not
    if (!active_proc) {
        // Get the proces id from the run queue
        if (queue_out(&run_queue, &pid) != 0) {
            // default to process id 0 (idle task)
            pid = 0;
        }

        active_proc = pid_to_proc(pid);
        kernel_log_trace("Scheduling process pid=%d, name=%s", active_proc->pid, active_proc->name);
=======
            // Unschedule the active process
            active_proc = NULL;
        }
    }
    // Check if we have a process scheduled or not
    if (!active_proc) {
        // Get the process id from the run queue
        // default to process id 0 (idle task) if a process can't be scheduled
        if (queue_out(&run_queue, &pid) != 0)
            pid = 0;

        // Update the active proc pointer
        active_proc = pid_to_proc(pid);

        // Fallback to idle task if needed
        if (!active_proc)
            active_proc = pid_to_proc(0);
>>>>>>> main
    }

    // Make sure we have a valid process at this point
    if (!active_proc) {
<<<<<<< syscall
        kernel_panic("Unable to schedule a process!");
    }

    // Ensure that the process state is correct
=======
        kernel_panic("scheduler: failed to select active process");
        return;
    }

    // Ensure that the process state is set
>>>>>>> main
    active_proc->state = ACTIVE;
}

/**
 * Adds a process to the scheduler
 * @param proc - pointer to the process entry
 */
void scheduler_add(proc_t *proc) {
<<<<<<< syscall
    if (!proc) {
        kernel_panic("Invalid process!");
    }

    proc->scheduler_queue = &run_queue;
    proc->state = IDLE;
    proc->cpu_time = 0;

    if (queue_in(proc->scheduler_queue, proc->pid) != 0) {
        kernel_panic("Unable to add the process to the scheduler");
    }
=======
    if (!proc)
        return;

    // Add only valid processes
    if (proc->state == NONE)
        return;

    // Do not add idle tasks to the run queue
    if (proc->pid == 0) {
        proc->state = IDLE;
        return;
    }

    // Add the process to the run queue
    if (queue_in(&run_queue, proc->pid) != 0) {
        kernel_log_error("scheduler: unable to enqueue process %d", proc->pid);
        return;
    }
    
    // Set the process state
    proc->state = IDLE;
>>>>>>> main
}

/**
 * Removes a process from the scheduler
 * @param proc - pointer to the process entry
 */
void scheduler_remove(proc_t *proc) {
<<<<<<< syscall
    int pid;

    if (!proc) {
        kernel_panic("Invalid process!");
        exit(1);
    }

    if (proc->scheduler_queue) {
        for (int i = 0; i < proc->scheduler_queue->size; i++) {
            if (queue_out(proc->scheduler_queue, &pid) != 0) {
                kernel_panic("Unable to queue out the process entry");
            }

            if (proc->pid == pid) {
                // Found the process
                // continue iterating so the run queue order is maintained
                continue;
            }

            // Add the item back to the run queue
            if (queue_in(proc->scheduler_queue, pid) != 0) {
                kernel_panic("Unable to queue process back to the run queue");
            }
        }

        // Set the queue to NULL since it does not exist in a queue any longer
        proc->scheduler_queue = NULL;
    }

    // If the process is the current process, ensure that the current
    // process is reset so a new process will be scheduled
    if (proc == active_proc) {
        active_proc = NULL;
    }
}

/**
 * Puts a process to sleep
 * @param proc - pointer to the process entry
 * @param time - number of ticks to sleep
 */
void scheduler_sleep(proc_t *proc, int time) {
    // Set the sleep time
    // Set the process state to SLEEPING
    // Remove the process from the scheduler
    // Add the proces to the sleep queue
=======
    int n;
    int pid;

    if (!proc)
        return;

    // Iterate through each the process queue
    // If the processis found, skip it; otherwise, ensure that each other process remains in the queue
    n = run_queue.size;

    for (int i = 0; i < n; i++) {
        if (queue_out(&run_queue, &pid) != 0)
            break;

        if (pid == proc->pid)
            continue;

        queue_in(&run_queue, pid);
    }

    // If the process is the active process, ensure that the active process is cleared so when the
    // scheduler runs again, it will select a new process to run
    if (proc == active_proc)
        active_proc = NULL;

    if (proc->state != NONE)
        proc->state = IDLE;
>>>>>>> main
}

/**
 * Initializes the scheduler, data structures, etc.
 */
void scheduler_init(void) {
    kernel_log_info("Initializing scheduler");

<<<<<<< syscall
    /* Initialize the run queue */
    queue_init(&run_queue);

    /* Register the timer callback */
    timer_callback_register(&scheduler_timer, 1, -1);
=======
    // Initialize any data structures or variables
    queue_init(&run_queue);

    // Register the timer callback (scheduler_timer) to run every tick
    timer_callback_register(scheduler_timer, 1, -1);
>>>>>>> main
}

