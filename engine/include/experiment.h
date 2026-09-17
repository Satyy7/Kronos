#ifndef KRONOS_EXPERIMENT_H
#define KRONOS_EXPERIMENT_H

#include <stdint.h>

#include "event.h"
#include "metrics.h"
#include "sync.h"


typedef enum
{
    WORKLOAD_COUNTER = 0,

    WORKLOAD_READERS_WRITERS,

    WORKLOAD_PRODUCER_CONSUMER

} workload_type_t;


typedef struct
{
    workload_type_t workload_type;

    int thread_count;

    uint64_t operations_per_thread;

    sync_type_t sync_type;


    /* Readers/Writers configuration */

    int reader_count;

    int writer_count;


    /* Producer/Consumer configuration */

    int producer_count;

    int consumer_count;

    int queue_capacity;

} experiment_config_t;


typedef struct
{
    uint64_t expected_counter;

    uint64_t actual_counter;

    metrics_t metrics;

    kronos_event_buffer_t events;

} experiment_result_t;


int experiment_run(
    const experiment_config_t *config,
    experiment_result_t *result
);

void experiment_set_live_output(int enabled);


#endif