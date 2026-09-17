#include "workload.h"

#include <stdint.h>

static uint64_t counter = 0;

static int counter_init(
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

static void counter_reset(void *arg)
{
    (void)arg;
    counter = 0;
}

static void *counter_worker(void *arg)
{
    workload_context_t *context =
        (workload_context_t *)arg;

    for (uint64_t i = 0;
         i < context->operations;
         i++)
    {
        if (context->sync->type == SYNC_ATOMIC)
        {
            kronos_sync_atomic_increment(context->sync);

            if (context->live_mode)
    {
        uint64_t current_value =
            kronos_sync_atomic_load(context->sync);

        kronos_event_record(
            context->events,
            context->thread_id,
            KRONOS_EVENT_COUNTER_INCREMENT,
            current_value
        );
    }
        }
        else
{
    kronos_sync_lock(
        context->sync,
        context->thread_id
    );

    counter++;

    uint64_t current_value = counter;

    kronos_sync_unlock(
        context->sync,
        context->thread_id
    );

    if (context->live_mode)
    {
        kronos_event_record(
            context->events,
            context->thread_id,
            KRONOS_EVENT_COUNTER_INCREMENT,
            current_value
        );
    }
}
    }

    return NULL;
}

static uint64_t counter_get_result(void *arg)
{
    (void)arg;
    return counter;
}

static void counter_cleanup(void *arg)
{
    (void)arg;
}

static const workload_t counter_workload =
{
    .name = "SHARED_COUNTER",
    .worker = counter_worker,
    .init = counter_init,
    .reset = counter_reset,
    .get_result = counter_get_result,
    .cleanup = counter_cleanup
};

const workload_t *kronos_counter_workload(void)
{
    return &counter_workload;
}