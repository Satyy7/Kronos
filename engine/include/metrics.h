#ifndef KRONOS_METRICS_H
#define KRONOS_METRICS_H

#include <stdint.h>
#include <time.h>

typedef struct
{
    struct timespec start_time;
    struct timespec end_time;

    uint64_t elapsed_ns;
    double elapsed_seconds;
    double throughput;
} metrics_t;

void metrics_start(metrics_t *metrics);

void metrics_stop(
    metrics_t *metrics,
    uint64_t operations
);

#endif