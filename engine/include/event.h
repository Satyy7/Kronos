#ifndef KRONOS_EVENT_H
#define KRONOS_EVENT_H

#include <stdint.h>
#include <stdatomic.h>

typedef enum
{
    KRONOS_EVENT_THREAD_START = 0,
    KRONOS_EVENT_THREAD_END,
    KRONOS_EVENT_SYNC_LOCK_WAIT,
    KRONOS_EVENT_SYNC_LOCK_ACQUIRE,
    KRONOS_EVENT_SYNC_LOCK_RELEASE,
    KRONOS_EVENT_SYNC_WAIT,
    KRONOS_EVENT_SYNC_SIGNAL,
    KRONOS_EVENT_QUEUE_PUSH,
    KRONOS_EVENT_QUEUE_POP
    KRONOS_EVENT_COUNTER_INCREMENT
} kronos_event_type_t;

typedef struct
{
    uint64_t timestamp_ns;
    int thread_id;
    kronos_event_type_t type;
    uint64_t value;
} kronos_event_t;

#define KRONOS_MAX_EVENTS 1000000

typedef struct
{
    kronos_event_t *events;
    atomic_uint_fast64_t count;
} kronos_event_buffer_t;

int kronos_event_buffer_init(
    kronos_event_buffer_t *buffer
);

void kronos_event_buffer_destroy(
    kronos_event_buffer_t *buffer
);

void kronos_event_record(
    kronos_event_buffer_t *buffer,
    int thread_id,
    kronos_event_type_t type,
    uint64_t value
);

uint64_t kronos_event_count(
    const kronos_event_buffer_t *buffer
);

const kronos_event_t *kronos_event_get(
    const kronos_event_buffer_t *buffer,
    uint64_t index
);

const char *kronos_event_type_name(
    kronos_event_type_t type
);

#endif