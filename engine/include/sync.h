#ifndef KRONOS_SYNC_H
#define KRONOS_SYNC_H

#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>

#include "event.h"

typedef enum
{
    SYNC_NONE = 0,
    SYNC_MUTEX,
    SYNC_SPINLOCK,
    SYNC_RWLOCK,
    SYNC_SEMAPHORE,
    SYNC_CONDVAR,
    SYNC_ATOMIC
} sync_type_t;

typedef struct
{
    sync_type_t type;

    pthread_mutex_t mutex;
    pthread_rwlock_t rwlock;
    pthread_cond_t cond_not_empty;
    pthread_cond_t cond_not_full;
    sem_t semaphore;

    atomic_flag spinlock;

    atomic_uint_fast64_t atomic_counter;

    kronos_event_buffer_t *events;

} kronos_sync_t;


/*
 * Initialize synchronization object.
 */
int kronos_sync_init(
    kronos_sync_t *sync,
    sync_type_t type,
    kronos_event_buffer_t *events
);


/*
 * Destroy synchronization object.
 */
void kronos_sync_destroy(
    kronos_sync_t *sync
);


/*
 * Generic exclusive lock operations.
 *
 * These operations generate:
 *
 * LOCK_WAIT
 * LOCK_ACQUIRE
 * LOCK_RELEASE
 */
void kronos_sync_lock(
    kronos_sync_t *sync,
    int thread_id
);

void kronos_sync_unlock(
    kronos_sync_t *sync,
    int thread_id
);


/*
 * Reader-specific operations.
 *
 * Used primarily by the Readers/Writers workload.
 */
void kronos_sync_read_lock(
    kronos_sync_t *sync,
    int thread_id
);

void kronos_sync_read_unlock(
    kronos_sync_t *sync,
    int thread_id
);


/*
 * Condition-variable operations.
 */
void kronos_sync_wait(
    kronos_sync_t *sync,
    int thread_id
);

void kronos_sync_signal(
    kronos_sync_t *sync,
    int thread_id
);


/*
 * Atomic counter operation.
 */
uint64_t kronos_sync_atomic_increment(
    kronos_sync_t *sync
);


/*
 * Read the current atomic counter value.
 */
uint64_t kronos_sync_atomic_load(
    kronos_sync_t *sync
);

void kronos_sync_broadcast(
    kronos_sync_t *sync,
    int thread_id
);

void kronos_sync_wait_not_empty(
    kronos_sync_t *sync,
    int thread_id
);

void kronos_sync_wait_not_full(
    kronos_sync_t *sync,
    int thread_id
);

void kronos_sync_signal_not_empty(
    kronos_sync_t *sync,
    int thread_id
);

void kronos_sync_signal_not_full(
    kronos_sync_t *sync,
    int thread_id
);

void kronos_sync_broadcast_not_empty(
    kronos_sync_t *sync,
    int thread_id
);


#endif