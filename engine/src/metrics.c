#include "metrics.h"

#include <time.h>

static uint64_t timespec_to_ns(
    const struct timespec *time)
{
    return (uint64_t)time->tv_sec * 1000000000ULL
           + (uint64_t)time->tv_nsec;
}

void metrics_start(metrics_t *metrics)
{
    clock_gettime(
        CLOCK_MONOTONIC,
        &metrics->start_time
    );
}

void metrics_stop(
    metrics_t *metrics,
    uint64_t operations)
{
    clock_gettime(
        CLOCK_MONOTONIC,
        &metrics->end_time
    );

    uint64_t start_ns =
        timespec_to_ns(&metrics->start_time);

    uint64_t end_ns =
        timespec_to_ns(&metrics->end_time);

    metrics->elapsed_ns = end_ns - start_ns;

    metrics->elapsed_seconds =
        (double)metrics->elapsed_ns / 1000000000.0;

    if (metrics->elapsed_seconds > 0.0)
    {
        metrics->throughput =
            (double)operations /
            metrics->elapsed_seconds;
    }
    else
    {
        metrics->throughput = 0.0;
    }
}