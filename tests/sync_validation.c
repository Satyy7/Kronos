#include "experiment.h"

#include <stdio.h>
#include <stdint.h>


static const char *sync_name(sync_type_t type)
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
    }

    return "UNKNOWN";
}


static int run_counter_test(
    sync_type_t sync_type
)
{
    experiment_config_t config = {0};
    experiment_result_t result = {0};


    config.workload_type =
        WORKLOAD_COUNTER;

    config.thread_count =
        4;

    config.operations_per_thread =
        100000;

    config.sync_type =
        sync_type;


    int status =
        experiment_run(
            &config,
            &result
        );


    if (status != 0)
    {
        printf(
            "[ERROR] COUNTER + %-10s "
            "experiment error\n",
            sync_name(sync_type)
        );

        return 1;
    }


    int correct =
        result.actual_counter ==
        result.expected_counter;


    /*
     * NONE is intentionally unsafe.
     *
     * A failed correctness check here is expected
     * because the workload contains a deliberate
     * data race.
     */
    if (sync_type == SYNC_NONE)
    {
        printf(
            "[EXPECTED] COUNTER + %-10s "
            "expected=%llu actual=%llu events=%llu\n",

            sync_name(sync_type),

            (unsigned long long)
                result.expected_counter,

            (unsigned long long)
                result.actual_counter,

            (unsigned long long)
                kronos_event_count(
                    &result.events
                )
        );
    }
    else
    {
        printf(
            "[%s] COUNTER + %-10s "
            "expected=%llu actual=%llu events=%llu\n",

            correct ? "PASS" : "FAIL",

            sync_name(sync_type),

            (unsigned long long)
                result.expected_counter,

            (unsigned long long)
                result.actual_counter,

            (unsigned long long)
                kronos_event_count(
                    &result.events
                )
        );
    }


    kronos_event_buffer_destroy(
        &result.events
    );


    if (sync_type == SYNC_NONE)
        return 0;


    return correct ? 0 : 1;
}


static int run_readers_writers_test(
    sync_type_t sync_type
)
{
    experiment_config_t config = {0};
    experiment_result_t result = {0};


    config.workload_type =
        WORKLOAD_READERS_WRITERS;

    config.thread_count =
        5;

    config.operations_per_thread =
        100000;

    config.sync_type =
        sync_type;

    config.reader_count =
        4;

    config.writer_count =
        1;


    int status =
        experiment_run(
            &config,
            &result
        );


    if (status != 0)
    {
        printf(
            "[ERROR] READERS_WRITERS + %-10s "
            "experiment error\n",
            sync_name(sync_type)
        );

        return 1;
    }


    /*
     * Only writers modify the shared counter.
     *
     * 1 writer × 100000 operations
     * = 100000 expected counter value.
     */
    uint64_t expected =
        (uint64_t)config.writer_count *
        config.operations_per_thread;


    int correct =
        result.actual_counter ==
        expected;


    printf(
        "[%s] READERS_WRITERS + %-10s "
        "expected=%llu actual=%llu events=%llu\n",

        correct ? "PASS" : "FAIL",

        sync_name(sync_type),

        (unsigned long long)
            expected,

        (unsigned long long)
            result.actual_counter,

        (unsigned long long)
            kronos_event_count(
                &result.events
            )
    );


    kronos_event_buffer_destroy(
        &result.events
    );


    return correct ? 0 : 1;
}


static int run_producer_consumer_test(void)
{
    experiment_config_t config = {0};
    experiment_result_t result = {0};


    config.workload_type =
        WORKLOAD_PRODUCER_CONSUMER;

    config.thread_count =
        4;

    config.operations_per_thread =
        1000;

    config.sync_type =
        SYNC_CONDVAR;

    config.producer_count =
        2;

    config.consumer_count =
        2;

    config.queue_capacity =
        2;


    int status =
        experiment_run(
            &config,
            &result
        );


    if (status != 0)
    {
        printf(
            "[ERROR] PRODUCER_CONSUMER + CONDVAR "
            "experiment error\n"
        );

        return 1;
    }


    int correct =
        result.actual_counter ==
        result.expected_counter;


    printf(
        "[%s] PRODUCER_CONSUMER + CONDVAR "
        "expected=%llu actual=%llu events=%llu\n",

        correct ? "PASS" : "FAIL",

        (unsigned long long)
            result.expected_counter,

        (unsigned long long)
            result.actual_counter,

        (unsigned long long)
            kronos_event_count(
                &result.events
            )
    );


    kronos_event_buffer_destroy(
        &result.events
    );


    return correct ? 0 : 1;
}


int main(void)
{
    int failures = 0;


    printf("\n");
    printf("========================================\n");
    printf("       KRONOS SYNCHRONIZATION TEST\n");
    printf("========================================\n");


    printf("\n");
    printf("--- COUNTER ---\n");


    failures +=
        run_counter_test(
            SYNC_NONE
        );

    failures +=
        run_counter_test(
            SYNC_MUTEX
        );

    failures +=
        run_counter_test(
            SYNC_SPINLOCK
        );

    failures +=
        run_counter_test(
            SYNC_RWLOCK
        );

    failures +=
        run_counter_test(
            SYNC_SEMAPHORE
        );

    failures +=
        run_counter_test(
            SYNC_CONDVAR
        );

    failures +=
        run_counter_test(
            SYNC_ATOMIC
        );


    printf("\n");
    printf("--- READERS / WRITERS ---\n");


    failures +=
        run_readers_writers_test(
            SYNC_MUTEX
        );

    failures +=
        run_readers_writers_test(
            SYNC_SPINLOCK
        );

    failures +=
        run_readers_writers_test(
            SYNC_RWLOCK
        );

    failures +=
        run_readers_writers_test(
            SYNC_SEMAPHORE
        );

    failures +=
        run_readers_writers_test(
            SYNC_CONDVAR
        );


    printf("\n");
    printf("--- PRODUCER / CONSUMER ---\n");


    failures +=
        run_producer_consumer_test();


    printf("\n");
    printf("========================================\n");


    if (failures == 0)
    {
        printf(
            "ALL CORE SYNCHRONIZATION TESTS PASSED\n"
        );
    }
    else
    {
        printf(
            "SYNCHRONIZATION TEST FAILURES: %d\n",
            failures
        );
    }


    printf("========================================\n\n");


    return failures == 0 ? 0 : 1;
}