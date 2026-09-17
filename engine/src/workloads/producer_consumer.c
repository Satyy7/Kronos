#include "workload.h"
#include <stdlib.h>

typedef struct
{
    uint64_t *buffer;
    int capacity;
    int head;
    int tail;
    int count;

    uint64_t total_produced;
    uint64_t total_consumed;

    int producer_count;
    int producers_finished;
} queue_t;

static queue_t queue;

static int producer_consumer_init(
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
    (void)reader_count;
    (void)writer_count;
    (void)operations_per_thread;

    if (
        producer_count <= 0 ||
        consumer_count <= 0 ||
        queue_capacity <= 0
    )
    {
        return -1;
    }

    queue.buffer = malloc(
        sizeof(uint64_t) * (size_t)queue_capacity
    );

    if (queue.buffer == NULL)
        return -1;

    queue.capacity = queue_capacity;
    queue.head = 0;
    queue.tail = 0;
    queue.count = 0;

    queue.total_produced = 0;
    queue.total_consumed = 0;

    queue.producer_count = producer_count;
    queue.producers_finished = 0;

    return 0;
}

static void producer_consumer_reset(
    void *arg
)
{
    (void)arg;

    queue.head = 0;
    queue.tail = 0;
    queue.count = 0;

    queue.total_produced = 0;
    queue.total_consumed = 0;

    queue.producers_finished = 0;
}

static void producer_consumer_cleanup(
    void *arg
)
{
    (void)arg;

    free(queue.buffer);

    queue.buffer = NULL;
    queue.capacity = 0;
    queue.head = 0;
    queue.tail = 0;
    queue.count = 0;

    queue.total_produced = 0;
    queue.total_consumed = 0;

    queue.producer_count = 0;
    queue.producers_finished = 0;
}

static void *producer_consumer_worker(
    void *arg
)
{
    workload_context_t *context =
        (workload_context_t *)arg;

    if (context->role == WORKLOAD_ROLE_PRODUCER)
    {
        for (
            uint64_t i = 0;
            i < context->operations;
            i++
        )
        {
            kronos_sync_lock(
                context->sync,
                context->thread_id
            );

            while (
                queue.count == queue.capacity
            )
            {
                kronos_sync_wait_not_full(
                    context->sync,
                    context->thread_id
                );
            }

            queue.buffer[queue.tail] = i;

            queue.tail =
                (queue.tail + 1) %
                queue.capacity;

            queue.count++;
            queue.total_produced++;

            kronos_event_record(
                context->events,
                context->thread_id,
                KRONOS_EVENT_QUEUE_PUSH,
                queue.count
            );

            kronos_sync_signal_not_empty(
                context->sync,
                context->thread_id
            );

            kronos_sync_unlock(
                context->sync,
                context->thread_id
            );
        }

        kronos_sync_lock(
            context->sync,
            context->thread_id
        );

        queue.producers_finished++;

        if (
            queue.producers_finished ==
            queue.producer_count
        )
        {
            kronos_sync_broadcast_not_empty(
                context->sync,
                context->thread_id
            );
        }

        kronos_sync_unlock(
            context->sync,
            context->thread_id
        );
    }
    else if (
        context->role == WORKLOAD_ROLE_CONSUMER
    )
    {
        while (1)
        {
            kronos_sync_lock(
                context->sync,
                context->thread_id
            );

            while (
                queue.count == 0 &&
                queue.producers_finished <
                    queue.producer_count
            )
            {
                kronos_sync_wait_not_empty(
                    context->sync,
                    context->thread_id
                );
            }

            if (
                queue.count == 0 &&
                queue.producers_finished ==
                    queue.producer_count
            )
            {
                kronos_sync_unlock(
                    context->sync,
                    context->thread_id
                );

                break;
            }

            (void)queue.buffer[queue.head];

            queue.head =
                (queue.head + 1) %
                queue.capacity;

            queue.count--;
            queue.total_consumed++;

            kronos_event_record(
                context->events,
                context->thread_id,
                KRONOS_EVENT_QUEUE_POP,
                queue.count
            );

            kronos_sync_signal_not_full(
                context->sync,
                context->thread_id
            );

            kronos_sync_unlock(
                context->sync,
                context->thread_id
            );
        }
    }

    return NULL;
}

static uint64_t producer_consumer_result(
    void *arg
)
{
    (void)arg;

    return queue.total_consumed;
}

static workload_t producer_consumer_workload =
{
    .name = "PRODUCER_CONSUMER",
    .worker = producer_consumer_worker,
    .init = producer_consumer_init,
    .reset = producer_consumer_reset,
    .get_result = producer_consumer_result,
    .cleanup = producer_consumer_cleanup
};

const workload_t *kronos_producer_consumer_workload(void)
{
    return &producer_consumer_workload;
}