/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 *
 * Kernel Mutexes
 */

#include <spede/string.h>

#include "kernel.h"
#include "kmutex.h"
#include "queue.h"
#include "scheduler.h"

// Table of all mutexes
mutex_t mutexes[MUTEX_MAX];

// Mutex ids to be allocated
queue_t mutex_queue;

/**
 * Initializes kernel mutex data structures
 * @return -1 on error, 0 on success
 */
int kmutexes_init(void) {
    kernel_log_info("Initializing kernel mutexes");

    // Initialize the mutex table
    memset(&mutexes, 0, sizeof(mutexes));

    // Initialize the mutex queue
    if (queue_init(&mutex_queue) == -1) {
        kernel_log_error("kmutex: failed to initialize mutex queue");
        return -1;
    }

    // Fill the mutex queue
    for (int i = 0; i < MUTEX_MAX; i++) {
        if (queue_in(&mutex_queue, i) == -1) {
            kernel_log_error("kmutex: failed to fill mutex queue");
            return -1;
        }
    }

    return 0;
}

/**
 * Allocates a mutex
 * @return -1 on error, otherwise the mutex id that was allocated
 */
int kmutex_init(void) {
    int id;
    mutex_t *mutex;

    // Obtain a mutex id from the mutex queue
    if (queue_out(&mutex_queue, &id) == -1) {
        kernel_log_error("kmutex: failed to obtain mutex id");
        return -1;
    }

    // Ensure that the id is within the valid range
    if (id < 0 || id >= MUTEX_MAX) {
        kernel_log_error("kmutex: mutex id %d is outside valid range", id);
        return -1;
    }

    // Pointer to the mutex table entry
    mutex = &mutexes[id];

    // Initialize the mutex data structure
    memset(mutex, 0, sizeof(mutex_t));

    mutex->allocated = 1;
    mutex->lock_count = 0;
    mutex->owner = NULL;

    init_splock(&mutex->lock);

    if (queue_init(&mutex->wait_queue) == -1) {
        kernel_log_error("kmutex: failed to initialize mutex wait queue");
        mutex->allocated = 0;
        queue_in(&mutex_queue, id);
        return -1;
    }

    return id;
}

/**
 * Frees the specified mutex
 * @param id - the mutex id
 * @return 0 on success, -1 on error
 */
int kmutex_destroy(int id) {
    mutex_t *mutex;

    if (id < 0 || id >= MUTEX_MAX) {
        kernel_log_error("kmutex: mutex id %d is outside valid range", id);
        return -1;
    }

    mutex = &mutexes[id];

    acquire_splock(&mutex->lock);

    if (!mutex->allocated) {
        release_splock(&mutex->lock);
        kernel_log_error("kmutex: mutex id %d is not allocated", id);
        return -1;
    }

    // Do not destroy a locked mutex or a mutex with waiting processes
    if (mutex->lock_count > 0 || !queue_is_empty(&mutex->wait_queue)) {
        release_splock(&mutex->lock);
        kernel_log_error("kmutex: mutex id %d is locked or has waiting processes", id);
        return -1;
    }

    release_splock(&mutex->lock);

    // Clear the memory for the data structure
    memset(mutex, 0, sizeof(mutex_t));

    // Add the id back into the mutex queue to be re-used later
    if (queue_in(&mutex_queue, id) == -1) {
        kernel_log_error("kmutex: failed to re-add mutex to mutex queue");
        return -1;
    }

    return 0;
}

/**
 * Locks the specified mutex
 * @param id - the mutex id
 * @return -1 on error, otherwise the current lock count
 */
int kmutex_lock(int id) {
    mutex_t *mutex;

    if (id < 0 || id >= MUTEX_MAX) {
        kernel_log_error("kmutex: invalid mutex id %d", id);
        return -1;
    }

    mutex = &mutexes[id];

    if (!active_proc) {
        kernel_log_error("kmutex: no active process");
        return -1;
    }

    acquire_splock(&mutex->lock);

    if (!mutex->allocated) {
        release_splock(&mutex->lock);
        kernel_log_error("kmutex: mutex id %d is not allocated", id);
        return -1;
    }

    // Recursive lock by same owner
    if (mutex->owner == active_proc) {
        mutex->lock_count++;
        int rc = mutex->lock_count;
        release_splock(&mutex->lock);
        return rc;
    }

    // Mutex is free
    if (mutex->lock_count == 0) {
        mutex->owner = active_proc;
        mutex->lock_count = 1;

        release_splock(&mutex->lock);
        return mutex->lock_count;
    }

    // Mutex is locked by another process, so block active process
    if (queue_in(&mutex->wait_queue, active_proc->pid) == -1) {
        release_splock(&mutex->lock);
        kernel_log_error("kmutex: failed to add active process to mutex wait queue");
        return -1;
    }

    active_proc->state = WAITING;

    release_splock(&mutex->lock);

    scheduler_remove(active_proc);

    return 0;
}

/**
 * Unlocks the specified mutex
 * @param id - the mutex id
 * @return -1 on error, otherwise the current lock count
 */
int kmutex_unlock(int id) {
    mutex_t *mutex;
    proc_t *next_proc = NULL;
    int next_pid;
    int rc;

    if (id < 0 || id >= MUTEX_MAX) {
        kernel_log_error("kmutex: mutex id %d is outside valid range", id);
        return -1;
    }

    if (!active_proc) {
        kernel_log_error("kmutex: no active process");
        return -1;
    }

    mutex = &mutexes[id];

    acquire_splock(&mutex->lock);

    if (!mutex->allocated) {
        release_splock(&mutex->lock);
        kernel_log_error("kmutex: mutex id %d is not allocated", id);
        return -1;
    }

    // If the mutex is not locked, there is nothing to do
    if (mutex->lock_count == 0) {
        release_splock(&mutex->lock);
        return 0;
    }

    // Only the owner may unlock
    if (mutex->owner != active_proc) {
        release_splock(&mutex->lock);
        kernel_log_error("kmutex: active process does not own mutex id %d", id);
        return -1;
    }

    // Recursive unlock
    mutex->lock_count--;

    if (mutex->lock_count > 0) {
        rc = mutex->lock_count;
        release_splock(&mutex->lock);
        return rc;
    }

    // Fully unlocked by current owner
    mutex->owner = NULL;

    // Wake the next waiting process, if one exists
    if (!queue_is_empty(&mutex->wait_queue)) {
        if (queue_out(&mutex->wait_queue, &next_pid) == -1) {
            release_splock(&mutex->lock);
            kernel_log_error("kmutex: failed to remove process from mutex wait queue");
            return -1;
        }

        next_proc = pid_to_proc(next_pid);

        if (next_proc == NULL) {
            release_splock(&mutex->lock);
            kernel_log_error("kmutex: failed to find process %d", next_pid);
            return -1;
        }

        mutex->owner = next_proc;
        mutex->lock_count = 1;
        next_proc->state = IDLE;
    }

    rc = mutex->lock_count;

    release_splock(&mutex->lock);

    if (next_proc != NULL) {
        scheduler_add(next_proc);
    }

    return rc;
}