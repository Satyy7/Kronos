#include "event.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static uint64_t kronos_now_ns(void)
{
    struct timespec ts;

    clock_gettime(
        CLOCK_MONOTONIC,
        &ts
    );

    return
        (uint64_t)ts.tv_sec * 1000000000ULL +
        (uint64_t)ts.tv_nsec;
}

const char *kronos_event_type_name(
    kronos_event_type_t type
)
{
    switch (type)
    {
        case KRONOS_EVENT_THREAD_START:
            return "THREAD_START";

        case KRONOS_EVENT_THREAD_END:
            return "THREAD_END";

        case KRONOS_EVENT_SYNC_LOCK_WAIT:
            return "LOCK_WAIT";

        case KRONOS_EVENT_SYNC_LOCK_ACQUIRE:
            return "LOCK_ACQUIRE";

        case KRONOS_EVENT_SYNC_LOCK_RELEASE:
            return "LOCK_RELEASE";

        case KRONOS_EVENT_SYNC_WAIT:
            return "WAIT";

        case KRONOS_EVENT_SYNC_SIGNAL:
            return "SIGNAL";

        case KRONOS_EVENT_QUEUE_PUSH:
            return "QUEUE_PUSH";

        case KRONOS_EVENT_QUEUE_POP:
            return "QUEUE_POP";
        
        case KRONOS_EVENT_COUNTER_INCREMENT:
                return "COUNTER_INCREMENT";

        default:
            return "UNKNOWN";
    }
}

int kronos_event_buffer_init(
    kronos_event_buffer_t *buffer
)
{
    if (buffer == NULL)
        return -1;

    buffer->events =
        calloc(
            KRONOS_MAX_EVENTS,
            sizeof(kronos_event_t)
        );

    if (buffer->events == NULL)
        return -1;

    atomic_init(
        &buffer->count,
        0
    );

    return 0;
}

void kronos_event_buffer_destroy(
    kronos_event_buffer_t *buffer
)
{
    if (buffer == NULL)
        return;

    free(buffer->events);

    buffer->events = NULL;

    atomic_store(
        &buffer->count,
        0
    );
}

void kronos_event_record(
    kronos_event_buffer_t *buffer,
    int thread_id,
    kronos_event_type_t type,
    uint64_t value
)
{
    if (
        buffer == NULL ||
        buffer->events == NULL
    )
    {
        return;
    }

    uint64_t index =
        atomic_fetch_add(
            &buffer->count,
            1
        );

    if (index >= KRONOS_MAX_EVENTS)
        return;

    kronos_event_t *event =
        &buffer->events[index];

    event->timestamp_ns =
        kronos_now_ns();

    event->thread_id =
        thread_id;

    event->type =
        type;

    event->value =
        value;
}

uint64_t kronos_event_count(
    const kronos_event_buffer_t *buffer
)
{
    if (buffer == NULL)
        return 0;

    uint64_t count =
        atomic_load(
            &buffer->count
        );

    if (count > KRONOS_MAX_EVENTS)
        count = KRONOS_MAX_EVENTS;

    return count;
}

const kronos_event_t *kronos_event_get(
    const kronos_event_buffer_t *buffer,
    uint64_t index
)
{
    if (
        buffer == NULL ||
        buffer->events == NULL
    )
    {
        return NULL;
    }

    if (
        index >=
        kronos_event_count(buffer)
    )
    {
        return NULL;
    }

    return &buffer->events[index];
}