#ifndef KRONOS_WORKLOAD_H
#define KRONOS_WORKLOAD_H

#include <stdint.h>

#include "event.h"
#include "sync.h"

typedef enum
{
    WORKLOAD_ROLE_WORKER = 0,
    WORKLOAD_ROLE_READER,
    WORKLOAD_ROLE_WRITER,
    WORKLOAD_ROLE_PRODUCER,
    WORKLOAD_ROLE_CONSUMER
} workload_role_t;

typedef struct
{
    int thread_id;
    uint64_t operations;

    kronos_sync_t *sync;
    kronos_event_buffer_t *events;

    workload_role_t role;

    int live_mode;

    void *(*workload)(void *arg);
} workload_context_t;

typedef void *(*workload_function_t)(void *arg);

typedef int (*workload_init_function_t)(
    int thread_count,
    uint64_t operations_per_thread,
    int reader_count,
    int writer_count,
    int producer_count,
    int consumer_count,
    int queue_capacity
);

typedef void (*workload_reset_function_t)(void *arg);

typedef uint64_t (*workload_result_function_t)(void *arg);

typedef void (*workload_cleanup_function_t)(void *arg);

typedef struct
{
    const char *name;

    workload_function_t worker;

    workload_init_function_t init;

    workload_reset_function_t reset;

    workload_result_function_t get_result;

    workload_cleanup_function_t cleanup;

} workload_t;

const workload_t *kronos_counter_workload(void);

const workload_t *kronos_readers_writers_workload(void);

const workload_t *kronos_producer_consumer_workload(void);

#endif