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
int kmutexes_init() {
    kernel_log_info("Initializing kernel mutexes");

    // Initialize the mutex table
    memset(&mutexes, 0, sizeof(mutexes));

    // Initialize the mutex queue
    if(queue_init(&mutex_queue) == -1) {
        kernel_log_error("kmutex: failed to initialize mutex queue");
        return -1;
    }

    // Fill the mutex queue
    for (int i = 0; i < MUTEX_MAX; i++) {
        if(queue_in(&mutex_queue, i) == -1) {
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

    // Obtain a mutex id from the mutex queue
    if(queue_out(&mutex_queue, &id) == -1) {
        kernel_log_error("kmutex: failed to obtain mutex id");
        return -1;
    }

    // Ensure that the id is within the valid range
    if(id < 0 || id >= MUTEX_MAX){
        kernel_log_error("kmutex: mutex id %d is outside valid range", id);
        return -1;
    }

    // Pointer to the mutex table entry
    mutex_t *mutex = &mutexes[id];

    // Initialize the mutex data structure (mutex_t + all members)
    memset(mutex, 0, sizeof(mutex_t));

    mutex->allocated = 1;
    mutex->lock_count = 0;

    init_splock(&mutex->lock);

    mutex->owner = NULL;
    
    if (queue_init(&mutex->wait_queue) == -1) {
        kernel_log_error("kmutex: failed to initialize mutex wait queue");
        mutex->allocated = 0;
        queue_in(&mutex_queue, id);
        return -1;
    }
    }

    // return the mutex id
    return id;
}

/**
 * Frees the specified mutex
 * @param id - the mutex id
 * @return 0 on success, -1 on error
 */
int kmutex_destroy(int id) {
    // look up the mutex in the mutex table
    if(id < 0 || id >= MUTEX_MAX){
        kernel_log_error("kmutex: mutex id %d is outside valid range", id);
        return -1;
    }

    mutex_t *mutex = &mutexes[id];

    if (!mutex->allocated) {
        kernel_log_error("kmutex: mutex id %d is not allocated", id);
        return -1;
    }

    // If the mutex is locked, prevent it from being destroyed (return error)
    if (mutex->lock_count > 0 || !queue_is_empty(&mutex->wait_queue)) {
        kernel_log_error("kmutex: mutex id %d is already locked", id);
        return -1;
    }

    // Clear the memory for the data structure
    memset(mutex, 0, sizeof(mutex_t));

    // Add the id back into the mutex queue to be re-used later
    if(queue_in(&mutex_queue, id) == -1) {
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
    // look up the mutex in the mutex table
    if(id < 0 || id >= MUTEX_MAX){
        kernel_log_error("kmutex: mutex id %d is outside valid range", id);
        return -1;
    }

    mutex_t *mutex = &mutexes[id];

    if (!mutex->allocated) {
        kernel_log_error("kmutex: mutex id %d is not allocated", id);
        return -1;
    }

    // Handle recursive locking
    if (mutex->owner == active_proc) {
        mutex->lock_count++;
        return mutex->lock_count;
    }

    // If the mutex is already locked
    //   1. Set the active process state to WAITING
    //   2. Add the process to the mutex wait queue (so it can take
    //      the mutex when it is unlocked)
    //   3. Remove the process from the scheduler, allow another
    //      process to be scheduled
    
    if(mutex->lock_count > 0){
        active_proc->state = WAITING;

        if(queue_in(&mutex->wait_queue, active_proc->pid) == -1){
            kernel_log_error("kmutex: failed to add active process to mutex wait queue");
            return -1;
        }

        scheduler_remove(active_proc);
    }

    // If the mutex is not locked
    //   1. set the mutex owner to the active process
    // Increment the lock count
    else{
        mutex->owner = active_proc;
        mutex->lock_count++;
    }

    // Return the mutex lock count
    return mutex->lock_count;
}

/**
 * Unlocks the specified mutex
 * @param id - the mutex id
 * @return -1 on error, otherwise the current lock count
 */
int kmutex_unlock(int id) {
    // look up the mutex in the mutex table
    if(id < 0 || id >= MUTEX_MAX){
        kernel_log_error("kmutex: mutex id %d is outside valid range", id);
        return -1;
    }

    mutex_t *mutex = &mutexes[id];

    if (!mutex->allocated) {
        kernel_log_error("kmutex: mutex id %d is not allocated", id);
        return -1;
    }

    // If the mutex is not locked, there is nothing to do
    if(mutex->lock_count == 0){
        return mutex->lock_count;
    }

    // Check lock ownership
    if(mutex->owner != active_proc) {
        kernel_log_error("kmutex: active process does not own the mutex id %d", id);
        return -1;
    }

    // Decrement the lock count
    mutex->lock_count--;

    if (mutex->lock_count > 0) {
        return mutex->lock_count;
    }

    // If there are no more locks held:
    //    1. clear the owner of the mutex
    mutex->owner = NULL;
    
    // If there are still locks held:
    //    1. Obtain a process from the mutex wait queue
    //    2. Add the process back to the scheduler
    //    3. set the owner of the of the mutex to the process
    if (!queue_is_empty(&mutex->wait_queue)) {
        int pid;

        if (queue_out(&mutex->wait_queue, &pid) == -1) {
            kernel_log_error("kmutex: failed to remove process from mutex wait queue");
            return -1;
        }

        proc_t *proc = pid_to_proc(pid);

        if (proc == NULL) {
            kernel_log_error("kmutex: failed to find process %d", pid);
            return -1;
        }

        scheduler_add(proc);
        mutex->owner = proc;
        mutex->lock_count = 1;
    }

    // return the mutex lock count
    return mutex->lock_count;
}
