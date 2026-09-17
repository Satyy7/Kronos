#include "workload.h"

#include <stdint.h>

static uint64_t shared_value = 0;

static int readers_writers_init(
    int thread_count,
    uint64_t operations_per_thread,
    int reader_count,
    int writer_count,
    int producer_count,
    int consumer_count,
    int queue_capacity
)
{
    (void)thread_count;
    (void)operations_per_thread;
    (void)reader_count;
    (void)writer_count;
    (void)producer_count;
    (void)consumer_count;
    (void)queue_capacity;

    return 0;
}

static void readers_writers_reset(void *arg)
{
    (void)arg;
    shared_value = 0;
}

static void reader_operation(
    workload_context_t *context
)
{
    kronos_sync_read_lock(
        context->sync,
        context->thread_id
    );

    uint64_t value = shared_value;
    (void)value;

    kronos_sync_read_unlock(
        context->sync,
        context->thread_id
    );
}

static void writer_operation(
    workload_context_t *context
)
{
    kronos_sync_lock(
        context->sync,
        context->thread_id
    );

    shared_value++;

    kronos_sync_unlock(
        context->sync,
        context->thread_id
    );
}

static void *readers_writers_worker(
    void *arg
)
{
    workload_context_t *context =
        (workload_context_t *)arg;

    if (context->role == WORKLOAD_ROLE_READER)
    {
        for (
            uint64_t i = 0;
            i < context->operations;
            i++
        )
        {
            reader_operation(context);
        }
    }
    else if (context->role == WORKLOAD_ROLE_WRITER)
    {
        for (
            uint64_t i = 0;
            i < context->operations;
            i++
        )
        {
            writer_operation(context);
        }
    }

    return NULL;
}

static uint64_t readers_writers_get_result(void *arg)
{
    (void)arg;
    return shared_value;
}

    static void readers_writers_cleanup(void *arg)
{
    (void)arg;
}

static const workload_t readers_writers_workload =
{
    .name = "READERS_WRITERS",
    .worker = readers_writers_worker,
    .init = readers_writers_init,
    .reset = readers_writers_reset,
    .get_result = readers_writers_get_result,
    .cleanup = readers_writers_cleanup
};

const workload_t *kronos_readers_writers_workload(void)
{
    return &readers_writers_workload;
}