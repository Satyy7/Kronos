#include "experiment.h"
#include "workload.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct
{
    pthread_t thread;
    workload_context_t context;
} worker_thread_t;

static int live_output_enabled = 0;

void experiment_set_live_output(int enabled)
{
    live_output_enabled = enabled ? 1 : 0;
}

static void emit_live_event(
    const kronos_event_t *event,
    uint64_t sequence
)
{
    if (
        !live_output_enabled ||
        event == NULL
    )
    {
        return;
    }

    flockfile(stderr);

    fprintf(
        stderr,
        "{\"type\":\"event\","
        "\"sequence\":%llu,"
        "\"timestamp_ns\":%llu,"
        "\"thread_id\":%d,"
        "\"event\":\"%s\","
        "\"value\":%llu}\n",

        (unsigned long long)sequence,

        (unsigned long long)
            event->timestamp_ns,

        event->thread_id,

        kronos_event_type_name(
            event->type
        ),

        (unsigned long long)
            event->value
    );

    fflush(stderr);

    funlockfile(stderr);
}

static void record_event(
    workload_context_t *context,
    kronos_event_type_t type,
    uint64_t value
)
{
    if (
        context == NULL ||
        context->events == NULL
    )
    {
        return;
    }

    kronos_event_record(
        context->events,
        context->thread_id,
        type,
        value
    );

    if (live_output_enabled)
    {
        uint64_t count =
            kronos_event_count(
                context->events
            );

        if (count > 0)
        {
            const kronos_event_t *event =
                kronos_event_get(
                    context->events,
                    count - 1
                );

            emit_live_event(
                event,
                count - 1
            );
        }
    }
}

static void *worker_entry(
    void *arg
)
{
    workload_context_t *context =
        (workload_context_t *)arg;

    record_event(
        context,
        KRONOS_EVENT_THREAD_START,
        0
    );

    void *result =
        context->workload(context);

    record_event(
        context,
        KRONOS_EVENT_THREAD_END,
        0
    );

    return result;
}

static const workload_t *select_workload(
    workload_type_t type
)
{
    switch (type)
    {
        case WORKLOAD_COUNTER:
            return kronos_counter_workload();

        case WORKLOAD_READERS_WRITERS:
            return kronos_readers_writers_workload();

        case WORKLOAD_PRODUCER_CONSUMER:
            return kronos_producer_consumer_workload();
    }

    return NULL;
}

int experiment_run(
    const experiment_config_t *config,
    experiment_result_t *result
)
{
    if (
        config == NULL ||
        result == NULL
    )
    {
        return -1;
    }

    const workload_t *workload =
        select_workload(
            config->workload_type
        );

    if (workload == NULL)
        return -1;

    result->expected_counter = 0;
    result->actual_counter = 0;
    result->events.events = NULL;

    if (
        workload->init(
            config->thread_count,
            config->operations_per_thread,
            config->reader_count,
            config->writer_count,
            config->producer_count,
            config->consumer_count,
            config->queue_capacity
        ) != 0
    )
    {
        return -1;
    }

    if (workload->reset != NULL)
        workload->reset(NULL);

    if (
        kronos_event_buffer_init(
            &result->events
        ) != 0
    )
    {
        if (workload->cleanup != NULL)
            workload->cleanup(NULL);

        return -1;
    }

    kronos_sync_t sync;

    if (
        kronos_sync_init(
            &sync,
            config->sync_type,
            &result->events
        ) != 0
    )
    {
        kronos_event_buffer_destroy(
            &result->events
        );

        if (workload->cleanup != NULL)
            workload->cleanup(NULL);

        return -1;
    }

    worker_thread_t *workers =
        calloc(
            (size_t)config->thread_count,
            sizeof(worker_thread_t)
        );

    if (workers == NULL)
    {
        kronos_sync_destroy(
            &sync
        );

        kronos_event_buffer_destroy(
            &result->events
        );

        if (workload->cleanup != NULL)
            workload->cleanup(NULL);

        return -1;
    }

    for (
        int i = 0;
        i < config->thread_count;
        i++
    )
    {
        workers[i].context.thread_id =
            i;

        workers[i].context.operations =
            config->operations_per_thread;

        workers[i].context.live_mode =
            live_output_enabled;

        workers[i].context.sync =
            &sync;

        workers[i].context.events =
            &result->events;

        workers[i].context.role =
            WORKLOAD_ROLE_WORKER;

        workers[i].context.workload =
            workload->worker;
    }

    if (
        config->workload_type ==
        WORKLOAD_READERS_WRITERS
    )
    {
        for (
            int i = 0;
            i < config->reader_count;
            i++
        )
        {
            workers[i].context.role =
                WORKLOAD_ROLE_READER;
        }

        for (
            int i = 0;
            i < config->writer_count;
            i++
        )
        {
            int index =
                config->reader_count + i;

            if (
                index <
                config->thread_count
            )
            {
                workers[index].context.role =
                    WORKLOAD_ROLE_WRITER;
            }
        }
    }
    else if (
        config->workload_type ==
        WORKLOAD_PRODUCER_CONSUMER
    )
    {
        for (
            int i = 0;
            i < config->producer_count;
            i++
        )
        {
            workers[i].context.role =
                WORKLOAD_ROLE_PRODUCER;
        }

        for (
            int i = 0;
            i < config->consumer_count;
            i++
        )
        {
            int index =
                config->producer_count + i;

            if (
                index <
                config->thread_count
            )
            {
                workers[index].context.role =
                    WORKLOAD_ROLE_CONSUMER;
            }
        }
    }

    metrics_start(
        &result->metrics
    );

    int created_threads = 0;

    for (
        int i = 0;
        i < config->thread_count;
        i++
    )
    {
        int error =
            pthread_create(
                &workers[i].thread,
                NULL,
                worker_entry,
                &workers[i].context
            );

        if (error != 0)
        {
            for (
                int j = 0;
                j < created_threads;
                j++
            )
            {
                pthread_join(
                    workers[j].thread,
                    NULL
                );
            }

            uint64_t measured_operations;

            if (
                config->workload_type ==
                WORKLOAD_PRODUCER_CONSUMER
            )
            {
                measured_operations =
                    (uint64_t)
                        config->producer_count *
                    config->operations_per_thread;
            }
            else
            {
                measured_operations =
                    (uint64_t)
                        config->thread_count *
                    config->operations_per_thread;
            }

            metrics_stop(
                &result->metrics,
                measured_operations
            );

            free(workers);

            kronos_sync_destroy(
                &sync
            );

            kronos_event_buffer_destroy(
                &result->events
            );

            if (workload->cleanup != NULL)
                workload->cleanup(NULL);

            return -1;
        }

        created_threads++;
    }

    for (
        int i = 0;
        i < config->thread_count;
        i++
    )
    {
        pthread_join(
            workers[i].thread,
            NULL
        );
    }

    uint64_t measured_operations;

    if (
        config->workload_type ==
        WORKLOAD_PRODUCER_CONSUMER
    )
    {
        measured_operations =
            (uint64_t)
                config->producer_count *
            config->operations_per_thread;
    }
    else
    {
        measured_operations =
            (uint64_t)
                config->thread_count *
            config->operations_per_thread;
    }

    metrics_stop(
        &result->metrics,
        measured_operations
    );

    if (
        config->workload_type ==
        WORKLOAD_PRODUCER_CONSUMER
    )
    {
        result->expected_counter =
            (uint64_t)
                config->producer_count *
            config->operations_per_thread;
    }
    else if (
        config->workload_type ==
        WORKLOAD_READERS_WRITERS
    )
    {
        result->expected_counter =
            (uint64_t)
                config->writer_count *
            config->operations_per_thread;
    }
    else
    {
        result->expected_counter =
            (uint64_t)
                config->thread_count *
            config->operations_per_thread;
    }

    if (
        config->workload_type ==
        WORKLOAD_COUNTER &&
        config->sync_type ==
        SYNC_ATOMIC
    )
    {
        result->actual_counter =
            kronos_sync_atomic_load(
                &sync
            );
    }
    else
    {
        result->actual_counter =
            workload->get_result(NULL);
    }

    free(workers);

    kronos_sync_destroy(
        &sync
    );

    if (workload->cleanup != NULL)
        workload->cleanup(NULL);

    return 0;
}