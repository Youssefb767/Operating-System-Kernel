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
queue_t sleep_queue;    // Sleep queue -> processes waiting to wake up

// From kproc.c
extern proc_t *active_proc;

/**
 * Scheduler timer callback
 */
void scheduler_timer(void) {
    int n;
    int pid;
    proc_t *proc;

    // Update the active process' run time and CPU time
    if (active_proc && active_proc->state == ACTIVE) {
        active_proc->run_time++;
        active_proc->cpu_time++;
    }

    // Wake sleeping processes if their wake time has arrived
    n = sleep_queue.size;

    for (int i = 0; i < n; i++) {
        if (queue_out(&sleep_queue, &pid) != 0) {
            break;
        }

        proc = pid_to_proc(pid);
        if (!proc || proc->state != SLEEPING) {
            continue;
        }

        if (timer_get_ticks() >= proc->sleep_time) {
            proc->sleep_time = 0;
            proc->state = IDLE;
            scheduler_add(proc);
        } else {
            proc->scheduler_queue = &sleep_queue;
            if (queue_in(&sleep_queue, pid) != 0) {
                kernel_log_error("scheduler: unable to requeue sleeping process %d", pid);
            }
        }
    }
}

/**
 * Executes the scheduler
 * Should ensure that `active_proc` is set to a valid process entry
 */
void scheduler_run(void) {
    int pid;

    // Ensure that processes not in the active state aren't still scheduled
    if (active_proc && (active_proc->state == NONE || active_proc->state == SLEEPING))
        active_proc = NULL;

    // Check if we have an active process
    if (active_proc) {
        // Check if the current process has exceeded it's time slice
        if (active_proc->cpu_time >= SCHEDULER_TIMESLICE) {
            // Reset the active time
            active_proc->cpu_time = 0;

            // If the process is not the idle task, add it back to the scheduler
            if (active_proc->pid != 0 && active_proc->state != SLEEPING)
                scheduler_add(active_proc);
            // Otherwise, simply set the state to IDLE
            else if (active_proc->pid == 0)
                active_proc->state = IDLE;

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
    }

    // Make sure we have a valid process at this point
    if (!active_proc) {
        kernel_panic("scheduler: failed to select active process");
        return;
    }

    // Ensure that the process state is set
    active_proc->state = ACTIVE;
    active_proc->scheduler_queue = NULL;
}

/**
 * Adds a process to the scheduler
 * @param proc - pointer to the process entry
 */
void scheduler_add(proc_t *proc) {
    if (!proc)
        return;

    // Add only valid processes
    if (proc->state == NONE || proc->state == SLEEPING)
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
    proc->scheduler_queue = &run_queue;
}

/**
 * Removes a process from the scheduler
 * @param proc - pointer to the process entry
 */
void scheduler_remove(proc_t *proc) {
    int n;
    int pid;
    queue_t *queue;

    if (!proc)
        return;

    // Iterate through each the process queue
    // If the processis found, skip it; otherwise, ensure that each other process remains in the queue
    queue = proc->scheduler_queue;
    if (!queue) {
        if (proc == active_proc)
            active_proc = NULL;
        return;
    }

    n = queue->size;

    for (int i = 0; i < n; i++) {
        if (queue_out(queue, &pid) != 0)
            break;

        if (pid == proc->pid)
            continue;

        queue_in(queue, pid);
    }

    // If the process is the active process, ensure that the active process is cleared so when the
    // scheduler runs again, it will select a new process to run
    if (proc == active_proc)
        active_proc = NULL;

    proc->scheduler_queue = NULL;

    if (proc->state != NONE && proc->state != SLEEPING)
        proc->state = IDLE;
}

/**
 * Initializes the scheduler, data structures, etc.
 */
void scheduler_init(void) {
    kernel_log_info("Initializing scheduler");

    // Initialize any data structures or variables
    queue_init(&run_queue);
    queue_init(&sleep_queue);

    // Register the timer callback (scheduler_timer) to run every tick
    timer_callback_register(scheduler_timer, 1, -1);
}

void scheduler_sleep(proc_t *proc, int seconds) {
    if (!proc || proc->state == NONE || seconds < 0) {
        return;
    }

    // Do not put idle to sleep
    if (proc->pid == 0) {
        return;
    }

    // Remove from run queue if present
    scheduler_remove(proc);

    // Clear as active if needed
    if (proc == active_proc) {
        active_proc = NULL;
    }

    // Set wakeup time in ticks
    proc->sleep_time = timer_get_ticks() + (seconds * 100);

    // Mark sleeping
    proc->state = SLEEPING;
    proc->scheduler_queue = &sleep_queue;

    // Add to sleep queue
    if (queue_in(&sleep_queue, proc->pid) != 0) {
        kernel_log_error("scheduler: unable to enqueue sleeping process %d", proc->pid);
        proc->state = IDLE;
        proc->sleep_time = 0;
        proc->scheduler_queue = NULL;
    }
}
