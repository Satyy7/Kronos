#include "sync.h"
#include <stdint.h>

int kronos_sync_init(
    kronos_sync_t *sync,
    sync_type_t type,
    kronos_event_buffer_t *events
)
{
    if (sync == NULL)
        return -1;

    sync->type = type;
    sync->events = events;

    switch (type)
    {
        case SYNC_NONE:
            return 0;

        case SYNC_MUTEX:
            return pthread_mutex_init(
                &sync->mutex,
                NULL
            );

        case SYNC_SPINLOCK:
            atomic_flag_clear(
                &sync->spinlock
            );
            return 0;

        case SYNC_RWLOCK:
            return pthread_rwlock_init(
                &sync->rwlock,
                NULL
            );

        case SYNC_SEMAPHORE:
            return sem_init(
                &sync->semaphore,
                0,
                1
            );

        case SYNC_CONDVAR:
        {
            int result;

            result = pthread_mutex_init(
                &sync->mutex,
                NULL
            );

            if (result != 0)
                return result;

            result = pthread_cond_init(
                &sync->cond_not_empty,
                NULL
            );

            if (result != 0)
            {
                pthread_mutex_destroy(
                    &sync->mutex
                );
                return result;
            }

            result = pthread_cond_init(
                &sync->cond_not_full,
                NULL
            );

            if (result != 0)
            {
                pthread_cond_destroy(
                    &sync->cond_not_empty
                );

                pthread_mutex_destroy(
                    &sync->mutex
                );

                return result;
            }

            return 0;
        }

        case SYNC_ATOMIC:
            atomic_init(
                &sync->atomic_counter,
                0
            );
            return 0;
    }

    return -1;
}

void kronos_sync_destroy(
    kronos_sync_t *sync
)
{
    if (sync == NULL)
        return;

    switch (sync->type)
    {
        case SYNC_NONE:
            break;

        case SYNC_MUTEX:
            pthread_mutex_destroy(
                &sync->mutex
            );
            break;

        case SYNC_SPINLOCK:
            break;

        case SYNC_RWLOCK:
            pthread_rwlock_destroy(
                &sync->rwlock
            );
            break;

        case SYNC_SEMAPHORE:
            sem_destroy(
                &sync->semaphore
            );
            break;

        case SYNC_CONDVAR:
            pthread_cond_destroy(
                &sync->cond_not_empty
            );

            pthread_cond_destroy(
                &sync->cond_not_full
            );

            pthread_mutex_destroy(
                &sync->mutex
            );
            break;

        case SYNC_ATOMIC:
            break;
    }
}

static void record_lock_wait(
    kronos_sync_t *sync,
    int thread_id
)
{
    if (sync == NULL || sync->events == NULL)
        return;

    kronos_event_record(
        sync->events,
        thread_id,
        KRONOS_EVENT_SYNC_LOCK_WAIT,
        0
    );
}

static void record_lock_acquire(
    kronos_sync_t *sync,
    int thread_id
)
{
    if (sync == NULL || sync->events == NULL)
        return;

    kronos_event_record(
        sync->events,
        thread_id,
        KRONOS_EVENT_SYNC_LOCK_ACQUIRE,
        0
    );
}

static void record_lock_release(
    kronos_sync_t *sync,
    int thread_id
)
{
    if (sync == NULL || sync->events == NULL)
        return;

    kronos_event_record(
        sync->events,
        thread_id,
        KRONOS_EVENT_SYNC_LOCK_RELEASE,
        0
    );
}

void kronos_sync_lock(
    kronos_sync_t *sync,
    int thread_id
)
{
    if (sync == NULL)
        return;

    if (sync->type == SYNC_NONE)
        return;

    if (sync->type == SYNC_ATOMIC)
        return;

    record_lock_wait(
        sync,
        thread_id
    );

    switch (sync->type)
    {
        case SYNC_MUTEX:
            pthread_mutex_lock(
                &sync->mutex
            );
            break;

        case SYNC_SPINLOCK:
            while (
                atomic_flag_test_and_set_explicit(
                    &sync->spinlock,
                    memory_order_acquire
                )
            )
            {
            }
            break;

        case SYNC_RWLOCK:
            pthread_rwlock_wrlock(
                &sync->rwlock
            );
            break;

        case SYNC_SEMAPHORE:
            sem_wait(
                &sync->semaphore
            );
            break;

        case SYNC_CONDVAR:
            pthread_mutex_lock(
                &sync->mutex
            );
            break;

        case SYNC_NONE:
        case SYNC_ATOMIC:
            break;
    }

    record_lock_acquire(
        sync,
        thread_id
    );
}

void kronos_sync_unlock(
    kronos_sync_t *sync,
    int thread_id
)
{
    if (sync == NULL)
        return;

    if (sync->type == SYNC_NONE)
        return;

    if (sync->type == SYNC_ATOMIC)
        return;

    switch (sync->type)
    {
        case SYNC_MUTEX:
            pthread_mutex_unlock(
                &sync->mutex
            );
            break;

        case SYNC_SPINLOCK:
            atomic_flag_clear_explicit(
                &sync->spinlock,
                memory_order_release
            );
            break;

        case SYNC_RWLOCK:
            pthread_rwlock_unlock(
                &sync->rwlock
            );
            break;

        case SYNC_SEMAPHORE:
            sem_post(
                &sync->semaphore
            );
            break;

        case SYNC_CONDVAR:
            pthread_mutex_unlock(
                &sync->mutex
            );
            break;

        case SYNC_NONE:
        case SYNC_ATOMIC:
            break;
    }

    record_lock_release(
        sync,
        thread_id
    );
}

void kronos_sync_read_lock(
    kronos_sync_t *sync,
    int thread_id
)
{
    if (sync == NULL)
        return;

    if (sync->type == SYNC_RWLOCK)
    {
        record_lock_wait(
            sync,
            thread_id
        );

        pthread_rwlock_rdlock(
            &sync->rwlock
        );

        record_lock_acquire(
            sync,
            thread_id
        );

        return;
    }

    kronos_sync_lock(
        sync,
        thread_id
    );
}

void kronos_sync_read_unlock(
    kronos_sync_t *sync,
    int thread_id
)
{
    if (sync == NULL)
        return;

    if (sync->type == SYNC_RWLOCK)
    {
        pthread_rwlock_unlock(
            &sync->rwlock
        );

        record_lock_release(
            sync,
            thread_id
        );

        return;
    }

    kronos_sync_unlock(
        sync,
        thread_id
    );
}

void kronos_sync_wait_not_empty(
    kronos_sync_t *sync,
    int thread_id
)
{
    if (
        sync == NULL ||
        sync->type != SYNC_CONDVAR
    )
    {
        return;
    }

    if (sync->events != NULL)
    {
        kronos_event_record(
            sync->events,
            thread_id,
            KRONOS_EVENT_SYNC_WAIT,
            0
        );
    }

    pthread_cond_wait(
        &sync->cond_not_empty,
        &sync->mutex
    );
}

void kronos_sync_wait_not_full(
    kronos_sync_t *sync,
    int thread_id
)
{
    if (
        sync == NULL ||
        sync->type != SYNC_CONDVAR
    )
    {
        return;
    }

    if (sync->events != NULL)
    {
        kronos_event_record(
            sync->events,
            thread_id,
            KRONOS_EVENT_SYNC_WAIT,
            0
        );
    }

    pthread_cond_wait(
        &sync->cond_not_full,
        &sync->mutex
    );
}

void kronos_sync_signal_not_empty(
    kronos_sync_t *sync,
    int thread_id
)
{
    if (
        sync == NULL ||
        sync->type != SYNC_CONDVAR
    )
    {
        return;
    }

    pthread_cond_signal(
        &sync->cond_not_empty
    );

    if (sync->events != NULL)
    {
        kronos_event_record(
            sync->events,
            thread_id,
            KRONOS_EVENT_SYNC_SIGNAL,
            0
        );
    }
}

void kronos_sync_signal_not_full(
    kronos_sync_t *sync,
    int thread_id
)
{
    if (
        sync == NULL ||
        sync->type != SYNC_CONDVAR
    )
    {
        return;
    }

    pthread_cond_signal(
        &sync->cond_not_full
    );

    if (sync->events != NULL)
    {
        kronos_event_record(
            sync->events,
            thread_id,
            KRONOS_EVENT_SYNC_SIGNAL,
            0
        );
    }
}

void kronos_sync_broadcast_not_empty(
    kronos_sync_t *sync,
    int thread_id
)
{
    if (
        sync == NULL ||
        sync->type != SYNC_CONDVAR
    )
    {
        return;
    }

    pthread_cond_broadcast(
        &sync->cond_not_empty
    );

    if (sync->events != NULL)
    {
        kronos_event_record(
            sync->events,
            thread_id,
            KRONOS_EVENT_SYNC_SIGNAL,
            0
        );
    }
}

void kronos_sync_wait(
    kronos_sync_t *sync,
    int thread_id
)
{
    if (sync == NULL)
        return;

    if (sync->type != SYNC_CONDVAR)
        return;

    kronos_sync_wait_not_empty(
        sync,
        thread_id
    );
}

void kronos_sync_signal(
    kronos_sync_t *sync,
    int thread_id
)
{
    if (sync == NULL)
        return;

    if (sync->type != SYNC_CONDVAR)
        return;

    kronos_sync_signal_not_empty(
        sync,
        thread_id
    );
}

void kronos_sync_broadcast(
    kronos_sync_t *sync,
    int thread_id
)
{
    if (sync == NULL)
        return;

    if (sync->type != SYNC_CONDVAR)
        return;

    kronos_sync_broadcast_not_empty(
        sync,
        thread_id
    );
}

uint64_t kronos_sync_atomic_increment(
    kronos_sync_t *sync
)
{
    if (sync == NULL)
        return 0;

    return atomic_fetch_add_explicit(
        &sync->atomic_counter,
        1,
        memory_order_relaxed
    ) + 1;
}

uint64_t kronos_sync_atomic_load(
    kronos_sync_t *sync
)
{
    if (sync == NULL)
        return 0;

    return atomic_load_explicit(
        &sync->atomic_counter,
        memory_order_relaxed
    );
}