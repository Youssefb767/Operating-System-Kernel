/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 *
 * Kernel Semaphores
 */

#include <spede/string.h>

#include "kernel.h"
#include "ksem.h"
#include "queue.h"
#include "scheduler.h"

// Table of all semaphores
sem_t semaphores[SEM_MAX];

// semaphore ids to be allocated
queue_t sem_queue;

/**
 * Initializes kernel semaphore data structures
 * @return -1 on error, 0 on success
 */
int ksemaphores_init() {
    kernel_log_info("Initializing kernel semaphores");

    // Initialize the semaphore table
    memset(semaphores, 0, sizeof(semaphores));

    // Initialize the semaphore queue
    if (queue_init(&sem_queue) != 0) {
        kernel_log_error("ksem: failed to initialize semaphore allocator queue");
        return -1;
    }

    // Fill the semaphore queue
    for (int i = 0; i < SEM_MAX; i++) {
        init_splock(&semaphores[i].lock);
        semaphores[i].count = -1;      // -1 means unused/unallocated

        if (queue_init(&semaphores[i].wait_queue) != 0) {
            kernel_log_error("ksem: failed to initialize wait queue %d", i);
            return -1;
        }

        if (queue_in(&sem_queue, i) != 0) {
            kernel_log_error("ksem: failed to add semaphore id %d to allocator", i);
            return -1;
        }
    }

    return 0;
}

/**
 * Allocates a semaphore
 * @param value - initial semaphore value
 * @return -1 on error, otherwise the semaphore id that was allocated
 */
int ksem_init(int value) {
    int id;
    sem_t *sem;

    if (value < 0) {
        kernel_log_error("ksem: invalid initial value %d", value);
        return -1;
    }

    // Obtain a semaphore id from the semaphore queue
    if (queue_out(&sem_queue, &id) != 0) {
        kernel_log_error("ksem: unable to allocate semaphore");
        return -1;
    }

    // Ensure that the id is within the valid range
    if (id < 0 || id >= SEM_MAX) {
        kernel_log_error("ksem: allocator returned invalid id %d", id);
        return -1;
    }

    // Initialize the semaphore data structure
    // semaphore table + all members (wait queue, allocated, count)
        // set count to initial value
    sem = &semaphores[id];

    memset(sem, 0, sizeof(sem_t));
    init_splock(&sem->lock);

    if (queue_init(&sem->wait_queue) != 0) {
        kernel_log_error("ksem: failed to initialize wait queue for id %d", id);
        queue_in(&sem_queue, id);
        return -1;
    }

    sem->count = value;

    kernel_log_debug("ksem: initialized semaphore %d count=%d", id, value);

    return id;
}

/**
 * Frees the specified semaphore
 * @param id - the semaphore id
 * @return 0 on success, -1 on error
 */
int ksem_destroy(int id) {
    sem_t *sem;

    // look up the semaphore in the semaphore table
    if (id < 0 || id >= SEM_MAX) {
        kernel_log_error("ksem:invalid semaphore id %d", id);
        return -1;
    }

    sem = &semaphores[id];

    if (sem->count < 0) {
        kernel_log_error("ksem: semaphore %d is not allocated", id);
        return -1;
    }

    acquire_splock(&sem->lock);

    // If the semaphore is locked, prevent it from being destroyed
    if (!queue_is_empty(&sem->wait_queue)) {
        release_splock(&sem->lock);
        kernel_log_error("ksem: cannot destroy semaphore %d with waiting processes", id);
        return -1;
    }

    release_splock(&sem->lock);

    // Clear the memory for the data structure
    memset(sem, 0, sizeof(sem_t));
    init_splock(&sem->lock);
    queue_init(&sem->wait_queue);
    sem->count = -1;       // mark unused

    // Add the id back into the semaphore queue to be re-used later
    if (queue_in(&sem_queue, id) != 0) {
        kernel_log_error("ksem: failed to return semaphore id %d to allocator", id);
        return -1;
    }

    kernel_log_debug("ksem: destroyed semaphore %d", id);
    
    return 0;
}

/**
 * Waits on the specified semaphore if it is held
 * @param id - the semaphore id
 * @return -1 on error, otherwise the current semaphore count
 */
int ksem_wait(int id) {
    sem_t *sem;
    proc_t *proc;
    int count;

    // look up the semaphore in the semaphore table
    if (id < 0 || id >= SEM_MAX) {
        kernel_log_error("ksem: invalid semaphore id %d", id);
        return -1;
    }

    sem = &semaphores[id];

    if (sem->count < 0) {
        kernel_log_error("ksem: semaphore %d is not allocated", id);
        return -1;
    }

    if (!active_proc) {
        kernel_log_error("ksem: no active process waiting on semaphore %d", id);
        return -1;
    }

    proc = active_proc;

    acquire_splock(&sem->lock);

    // If the semaphore count is 0, then the process must wait
    if (sem->count == 0) {
        // add to the semaphore's wait queue
        if (queue_in(&sem->wait_queue, proc->pid) != 0) {
            release_splock(&sem->lock);
            kernel_log_error("ksem: unable to queue process %d on semaphore %d", proc->pid, id);
            return -1;
        }

        // Set the state to WAITING
        proc->state = WAITING;

        //kernel_log_debug("ksem: wait id=%d pid=%d blocked count=%d waiters=%d", id, proc->pid, sem->count, sem->wait_queue.size);

        count = sem->count;

        release_splock(&sem->lock);

        // remove from the scheduler
        scheduler_remove(proc);

        return count;
    }

    // If the semaphore count is > 0
    // Decrement the count
    sem->count--;
    count = sem->count;

    release_splock(&sem->lock);

    // Return the current semaphore count
    return count;
}

/**
 * Posts the specified semaphore
 * @param id - the semaphore id
 * @return -1 on error, otherwise the current semaphore count
 */
int ksem_post(int id) {
    sem_t *sem;
    proc_t *proc;
    int pid;
    int count;

    // look up the semaphore in the semaphore table
    if (id < 0 || id >= SEM_MAX) {
        kernel_log_error("ksem: invalid semaphore id %d", id);
        return -1;
    }

    sem = &semaphores[id];

    if (sem->count < 0) {
        kernel_log_error("ksem: semaphore %d is not allocated", id);
        return -1;
    }

    acquire_splock(&sem->lock);

    // increment the semaphore count
    sem->count++;

    // check if any processes are waiting on the semaphore (semaphore wait queue)
    while (!queue_is_empty(&sem->wait_queue)) {
        // if so, queue out and add to the scheduler
        if (queue_out(&sem->wait_queue, &pid) != 0) {
            break;
        }

        proc = pid_to_proc(pid);
        if (!proc || proc->state == NONE) {
            continue;
        }

        // decrement the semaphore count
        sem->count--;

        count = sem->count;
        proc->state = IDLE;

        //kernel_log_debug("ksem: post id=%d woke pid=%d count=%d waiters=%d", id, pid, sem->count, sem->wait_queue.size);

        release_splock(&sem->lock);

        scheduler_add(proc);

        return count;
    }

    count = sem->count;

    release_splock(&sem->lock);

    // return current semaphore count
    return count;
}
