#include "kronos.h"

#include "experiment.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *event_name(
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

        default:
            return "UNKNOWN";
    }
}

static const char *sync_name(
    sync_type_t type
)
{
    switch (type)
    {
        case SYNC_NONE:
            return "NONE";

        case SYNC_MUTEX:
            return "MUTEX";

        case SYNC_SPINLOCK:
            return "SPINLOCK";

        case SYNC_RWLOCK:
            return "RWLOCK";

        case SYNC_SEMAPHORE:
            return "SEMAPHORE";

        case SYNC_CONDVAR:
            return "CONDVAR";

        case SYNC_ATOMIC:
            return "ATOMIC";

        default:
            return "UNKNOWN";
    }
}

static const char *workload_name(
    workload_type_t type
)
{
    switch (type)
    {
        case WORKLOAD_COUNTER:
            return "COUNTER";

        case WORKLOAD_READERS_WRITERS:
            return "READERS_WRITERS";

        case WORKLOAD_PRODUCER_CONSUMER:
            return "PRODUCER_CONSUMER";

        default:
            return "UNKNOWN";
    }
}

static void print_usage(
    const char *program
)
{
    printf(
        "Usage: %s [options]\n\n"
        "Options:\n"
        "  --workload <name>       COUNTER | READERS_WRITERS | PRODUCER_CONSUMER\n"
        "  --sync <name>           NONE | MUTEX | SPINLOCK | RWLOCK | SEMAPHORE | CONDVAR | ATOMIC\n"
        "  --threads <count>       Worker thread count\n"
        "  --operations <count>    Operations per thread\n"
        "  --readers <count>       Reader thread count\n"
        "  --writers <count>       Writer thread count\n"
        "  --producers <count>     Producer thread count\n"
        "  --consumers <count>     Consumer thread count\n"
        "  --queue-capacity <n>    Bounded queue capacity\n"
        "  --json                  Output final result as JSON\n"
        "  --live                  Output event telemetry to stderr\n"
        "  --help                  Show this help message\n",
        program
    );
}

static int parse_workload(
    const char *value,
    workload_type_t *workload
)
{
    if (strcmp(value, "COUNTER") == 0)
    {
        *workload = WORKLOAD_COUNTER;
        return 0;
    }

    if (strcmp(value, "READERS_WRITERS") == 0)
    {
        *workload = WORKLOAD_READERS_WRITERS;
        return 0;
    }

    if (strcmp(value, "PRODUCER_CONSUMER") == 0)
    {
        *workload = WORKLOAD_PRODUCER_CONSUMER;
        return 0;
    }

    return -1;
}

static int parse_sync(
    const char *value,
    sync_type_t *sync
)
{
    if (strcmp(value, "NONE") == 0)
    {
        *sync = SYNC_NONE;
        return 0;
    }

    if (strcmp(value, "MUTEX") == 0)
    {
        *sync = SYNC_MUTEX;
        return 0;
    }

    if (strcmp(value, "SPINLOCK") == 0)
    {
        *sync = SYNC_SPINLOCK;
        return 0;
    }

    if (strcmp(value, "RWLOCK") == 0)
    {
        *sync = SYNC_RWLOCK;
        return 0;
    }

    if (strcmp(value, "SEMAPHORE") == 0)
    {
        *sync = SYNC_SEMAPHORE;
        return 0;
    }

    if (strcmp(value, "CONDVAR") == 0)
    {
        *sync = SYNC_CONDVAR;
        return 0;
    }

    if (strcmp(value, "ATOMIC") == 0)
    {
        *sync = SYNC_ATOMIC;
        return 0;
    }

    return -1;
}

static int parse_positive_int(
    const char *value,
    int *result
)
{
    char *end = NULL;

    long parsed =
        strtol(
            value,
            &end,
            10
        );

    if (
        value[0] == '\0' ||
        end == value ||
        *end != '\0' ||
        parsed <= 0 ||
        parsed > 1000000
    )
    {
        return -1;
    }

    *result = (int)parsed;

    return 0;
}

static int parse_positive_uint64(
    const char *value,
    uint64_t *result
)
{
    char *end = NULL;

    unsigned long long parsed =
        strtoull(
            value,
            &end,
            10
        );

    if (
        value[0] == '\0' ||
        end == value ||
        *end != '\0' ||
        parsed == 0
    )
    {
        return -1;
    }

    *result = (uint64_t)parsed;

    return 0;
}

static int parse_arguments(
    int argc,
    char **argv,
    experiment_config_t *config,
    int *json_output,
    int *live_output
)
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--help") == 0)
        {
            print_usage(argv[0]);
            return 1;
        }

        if (strcmp(argv[i], "--json") == 0)
        {
            *json_output = 1;
            continue;
        }

        if (strcmp(argv[i], "--live") == 0)
        {
            *live_output = 1;
            continue;
        }

        if (i + 1 >= argc)
        {
            fprintf(
                stderr,
                "Missing value for option: %s\n",
                argv[i]
            );

            return -1;
        }

        if (strcmp(argv[i], "--workload") == 0)
        {
            if (
                parse_workload(
                    argv[++i],
                    &config->workload_type
                ) != 0
            )
            {
                fprintf(
                    stderr,
                    "Invalid workload: %s\n",
                    argv[i]
                );

                return -1;
            }

            continue;
        }

        if (strcmp(argv[i], "--sync") == 0)
        {
            if (
                parse_sync(
                    argv[++i],
                    &config->sync_type
                ) != 0
            )
            {
                fprintf(
                    stderr,
                    "Invalid synchronization type: %s\n",
                    argv[i]
                );

                return -1;
            }

            continue;
        }

        if (strcmp(argv[i], "--threads") == 0)
        {
            if (
                parse_positive_int(
                    argv[++i],
                    &config->thread_count
                ) != 0
            )
            {
                fprintf(
                    stderr,
                    "Invalid thread count: %s\n",
                    argv[i]
                );

                return -1;
            }

            continue;
        }

        if (strcmp(argv[i], "--operations") == 0)
        {
            if (
                parse_positive_uint64(
                    argv[++i],
                    &config->operations_per_thread
                ) != 0
            )
            {
                fprintf(
                    stderr,
                    "Invalid operation count: %s\n",
                    argv[i]
                );

                return -1;
            }

            continue;
        }

        if (strcmp(argv[i], "--readers") == 0)
        {
            if (
                parse_positive_int(
                    argv[++i],
                    &config->reader_count
                ) != 0
            )
            {
                fprintf(
                    stderr,
                    "Invalid reader count: %s\n",
                    argv[i]
                );

                return -1;
            }

            continue;
        }

        if (strcmp(argv[i], "--writers") == 0)
        {
            if (
                parse_positive_int(
                    argv[++i],
                    &config->writer_count
                ) != 0
            )
            {
                fprintf(
                    stderr,
                    "Invalid writer count: %s\n",
                    argv[i]
                );

                return -1;
            }

            continue;
        }

        if (strcmp(argv[i], "--producers") == 0)
        {
            if (
                parse_positive_int(
                    argv[++i],
                    &config->producer_count
                ) != 0
            )
            {
                fprintf(
                    stderr,
                    "Invalid producer count: %s\n",
                    argv[i]
                );

                return -1;
            }

            continue;
        }

        if (strcmp(argv[i], "--consumers") == 0)
        {
            if (
                parse_positive_int(
                    argv[++i],
                    &config->consumer_count
                ) != 0
            )
            {
                fprintf(
                    stderr,
                    "Invalid consumer count: %s\n",
                    argv[i]
                );

                return -1;
            }

            continue;
        }

        if (strcmp(argv[i], "--queue-capacity") == 0)
        {
            if (
                parse_positive_int(
                    argv[++i],
                    &config->queue_capacity
                ) != 0
            )
            {
                fprintf(
                    stderr,
                    "Invalid queue capacity: %s\n",
                    argv[i]
                );

                return -1;
            }

            continue;
        }

        fprintf(
            stderr,
            "Unknown option: %s\n",
            argv[i]
        );

        return -1;
    }

    return 0;
}

static int validate_config(
    const experiment_config_t *config
)
{
    if (config->workload_type == WORKLOAD_COUNTER)
    {
        if (config->thread_count <= 0)
        {
            fprintf(
                stderr,
                "Counter requires at least one thread\n"
            );

            return -1;
        }

        if (
            config->sync_type < SYNC_NONE ||
            config->sync_type > SYNC_ATOMIC
        )
        {
            fprintf(
                stderr,
                "Invalid synchronization type for Counter\n"
            );

            return -1;
        }

        return 0;
    }

    if (
        config->workload_type ==
        WORKLOAD_READERS_WRITERS
    )
    {
        if (config->reader_count <= 0)
        {
            fprintf(
                stderr,
                "Readers/Writers requires at least one reader\n"
            );

            return -1;
        }

        if (config->writer_count <= 0)
        {
            fprintf(
                stderr,
                "Readers/Writers requires at least one writer\n"
            );

            return -1;
        }

        if (config->sync_type == SYNC_ATOMIC)
        {
            fprintf(
                stderr,
                "Readers/Writers does not support ATOMIC synchronization\n"
            );

            return -1;
        }

        return 0;
    }

    if (
        config->workload_type ==
        WORKLOAD_PRODUCER_CONSUMER
    )
    {
        if (config->producer_count <= 0)
        {
            fprintf(
                stderr,
                "Producer/Consumer requires at least one producer\n"
            );

            return -1;
        }

        if (config->consumer_count <= 0)
        {
            fprintf(
                stderr,
                "Producer/Consumer requires at least one consumer\n"
            );

            return -1;
        }

        if (config->queue_capacity <= 0)
        {
            fprintf(
                stderr,
                "Queue capacity must be greater than zero\n"
            );

            return -1;
        }

        if (config->sync_type != SYNC_CONDVAR)
        {
            fprintf(
                stderr,
                "Producer/Consumer requires CONDVAR synchronization\n"
            );

            return -1;
        }

        return 0;
    }

    fprintf(
        stderr,
        "Invalid workload type\n"
    );

    return -1;
}

static void normalize_config(
    experiment_config_t *config
)
{
    if (
        config->workload_type ==
        WORKLOAD_READERS_WRITERS
    )
    {
        config->thread_count =
            config->reader_count +
            config->writer_count;
    }
    else if (
        config->workload_type ==
        WORKLOAD_PRODUCER_CONSUMER
    )
    {
        config->thread_count =
            config->producer_count +
            config->consumer_count;
    }
}

static void print_live_event(
    const kronos_event_t *event,
    uint64_t sequence
)
{
    if (event == NULL)
        return;

    fprintf(
        stderr,
        "{\"type\":\"event\","
        "\"sequence\":%llu,"
        "\"timestamp_ns\":%llu,"
        "\"thread_id\":%d,"
        "\"event\":\"%s\","
        "\"value\":%llu}\n",

        (unsigned long long)
            sequence,

        (unsigned long long)
            event->timestamp_ns,

        event->thread_id,

        event_name(
            event->type
        ),

        (unsigned long long)
            event->value
    );

    fflush(stderr);
}

static void print_live_events(
    const experiment_result_t *result
)
{
    if (result == NULL)
        return;

    uint64_t count =
        kronos_event_count(
            &result->events
        );

    for (
        uint64_t i = 0;
        i < count;
        i++
    )
    {
        const kronos_event_t *event =
            kronos_event_get(
                &result->events,
                i
            );

        print_live_event(
            event,
            i
        );
    }
}

static void print_json_result(
    const experiment_config_t *config,
    const experiment_result_t *result
)
{
    uint64_t event_count =
        kronos_event_count(
            &result->events
        );

    printf(
        "{\n"
        "  \"workload\": \"%s\",\n"
        "  \"synchronization\": \"%s\",\n"
        "  \"thread_count\": %d,\n"
        "  \"operations_per_thread\": %llu,\n",

        workload_name(
            config->workload_type
        ),

        sync_name(
            config->sync_type
        ),

        config->thread_count,

        (unsigned long long)
            config->operations_per_thread
    );

    if (
        config->workload_type ==
        WORKLOAD_READERS_WRITERS
    )
    {
        printf(
            "  \"readers\": %d,\n"
            "  \"writers\": %d,\n",

            config->reader_count,
            config->writer_count
        );
    }

    if (
        config->workload_type ==
        WORKLOAD_PRODUCER_CONSUMER
    )
    {
        printf(
            "  \"producers\": %d,\n"
            "  \"consumers\": %d,\n"
            "  \"queue_capacity\": %d,\n",

            config->producer_count,
            config->consumer_count,
            config->queue_capacity
        );
    }

    printf(
        "  \"expected_result\": %llu,\n"
        "  \"actual_result\": %llu,\n"
        "  \"elapsed_seconds\": %.9f,\n"
        "  \"throughput\": %.2f,\n"
        "  \"events_recorded\": %llu\n"
        "}\n",

        (unsigned long long)
            result->expected_counter,

        (unsigned long long)
            result->actual_counter,

        result->metrics.elapsed_seconds,

        result->metrics.throughput,

        (unsigned long long)
            event_count
    );

    fflush(stdout);
}

int kronos_run(
    int argc,
    char **argv
)
{
    experiment_config_t config =
    {
        .workload_type =
            WORKLOAD_PRODUCER_CONSUMER,

        .thread_count = 4,

        .operations_per_thread =
            1000,

        .sync_type =
            SYNC_CONDVAR,

        .reader_count = 0,

        .writer_count = 0,

        .producer_count = 2,

        .consumer_count = 2,

        .queue_capacity = 2
    };

    int json_output = 0;
    int live_output = 0;

    int parse_result =
        parse_arguments(
            argc,
            argv,
            &config,
            &json_output,
            &live_output
        );

    if (parse_result == 1)
        return 0;

    if (parse_result != 0)
    {
        print_usage(argv[0]);
        return 1;
    }

    if (validate_config(&config) != 0)
        return 1;

    normalize_config(&config);

    experiment_set_live_output(
    live_output
    );

    experiment_result_t result;

    int error =
        experiment_run(
            &config,
            &result
        );

    if (error != 0)
    {
        fprintf(
            stderr,
            "Experiment failed\n"
        );

        return 1;
    }

    /*
     * Live telemetry is written to stderr.
     *
     * This keeps stdout clean so that --json remains
     * valid JSON for the Next.js API bridge.
     *
     * At this stage events are emitted after experiment_run()
     * returns. True during-execution streaming will be connected
     * in experiment.c in the next step.
     */
    if (live_output)
    {
        print_live_events(
            &result
        );
    }

    if (json_output)
    {
        print_json_result(
            &config,
            &result
        );

        kronos_event_buffer_destroy(
            &result.events
        );

        return 0;
    }

    printf(
        "\n=== Experiment Complete ===\n"
    );

    printf(
        "Workload: %s\n",
        workload_name(
            config.workload_type
        )
    );

    printf(
        "Synchronization: %s\n",
        sync_name(
            config.sync_type
        )
    );

    int total_threads;

    if (
        config.workload_type ==
        WORKLOAD_READERS_WRITERS
    )
    {
        total_threads =
            config.reader_count +
            config.writer_count;
    }
    else if (
        config.workload_type ==
        WORKLOAD_PRODUCER_CONSUMER
    )
    {
        total_threads =
            config.producer_count +
            config.consumer_count;
    }
    else
    {
        total_threads =
            config.thread_count;
    }

    printf(
        "Total threads: %d\n",
        total_threads
    );

    if (
        config.workload_type ==
        WORKLOAD_READERS_WRITERS
    )
    {
        printf(
            "Readers: %d\n",
            config.reader_count
        );

        printf(
            "Writers: %d\n",
            config.writer_count
        );
    }

    if (
        config.workload_type ==
        WORKLOAD_PRODUCER_CONSUMER
    )
    {
        printf(
            "Producers: %d\n",
            config.producer_count
        );

        printf(
            "Consumers: %d\n",
            config.consumer_count
        );

        printf(
            "Queue capacity: %d\n",
            config.queue_capacity
        );
    }

    printf(
        "Operations/thread: %llu\n",
        (unsigned long long)
            config.operations_per_thread
    );

    printf(
        "Expected result: %llu\n",
        (unsigned long long)
            result.expected_counter
    );

    printf(
        "Actual result:   %llu\n",
        (unsigned long long)
            result.actual_counter
    );

    printf(
        "Elapsed time:     %.6f seconds\n",
        result.metrics.elapsed_seconds
    );

    printf(
        "Throughput:       %.2f operations/sec\n",
        result.metrics.throughput
    );

    uint64_t event_count =
        kronos_event_count(
            &result.events
        );

    printf(
        "Events recorded:  %llu\n",
        (unsigned long long)
            event_count
    );

    printf(
        "\n=== First Events ===\n"
    );

    uint64_t display_count =
        event_count < 20
            ? event_count
            : 20;

    for (
        uint64_t i = 0;
        i < display_count;
        i++
    )
    {
        const kronos_event_t *event =
            kronos_event_get(
                &result.events,
                i
            );

        if (event == NULL)
            continue;

        printf(
            "[%llu] time=%llu ns "
            "thread=%d "
            "event=%s "
            "value=%llu\n",

            (unsigned long long)i,

            (unsigned long long)
                event->timestamp_ns,

            event->thread_id,

            event_name(
                event->type
            ),

            (unsigned long long)
                event->value
        );
    }

    kronos_event_buffer_destroy(
        &result.events
    );

    return 0;
}