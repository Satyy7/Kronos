# KRONOS

### Real-Time Multicore Concurrency Lab

Kronos is a **native C11/POSIX multithreading experimentation platform** for studying how concurrent threads behave under different synchronization mechanisms.

Instead of only learning synchronization APIs such as mutexes, spinlocks, semaphores, and atomics theoretically, Kronos **executes real concurrent workloads, instruments thread and synchronization activity, measures performance, verifies results, and visualizes the execution through a web-based control room.**

> **Execute concurrency → Instrument execution → Measure performance → Visualize behavior**

---

## What Kronos Does

Kronos provides a controlled environment where the same concurrent workload can be executed using different synchronization strategies.

### Supported Workloads

#### 1. Shared Counter

Multiple threads repeatedly update a shared counter.

The workload can be executed using:

- No synchronization
- Mutex
- Spinlock
- Read-write lock
- Semaphore
- Condition variable
- C11 atomic operations

The final counter is compared against the expected number of updates, while execution time and throughput are measured.

---

#### 2. Readers / Writers

Multiple reader and writer threads operate on shared data.

- Readers acquire read access.
- Writers require exclusive access.
- Synchronization controls concurrent access to the shared state.

This workload is useful for observing how different synchronization mechanisms behave when multiple threads are simultaneously reading and modifying shared data.

---

#### 3. Producer / Consumer

Producer and consumer threads operate concurrently on a shared bounded queue.

Producers add work to the queue while consumers remove it.

Synchronization is required when:

- The queue becomes full.
- The queue becomes empty.
- Producers need to wait for consumers.
- Consumers need to wait for producers.

Kronos records these synchronization and queue events so the execution can be observed.

---

# Synchronization Layer

Kronos provides a common synchronization layer over multiple synchronization mechanisms:

| Synchronization | Purpose |
|---|---|
| `NONE` | Execute without synchronization |
| `MUTEX` | Mutual exclusion |
| `SPINLOCK` | Busy-wait based locking |
| `RWLOCK` | Separate reader and writer access |
| `SEMAPHORE` | Synchronization using a semaphore |
| `CONDVAR` | Waiting and signalling between threads |
| `ATOMIC` | Lock-free atomic operations |

The workload does not need to know the implementation details of every synchronization primitive.

Instead, the synchronization layer provides common operations for:

- Initialization
- Lock acquisition
- Lock release
- Read locking
- Condition-variable waiting
- Condition-variable signalling
- Atomic operations
- Cleanup

This allows the **same workload to be executed with different synchronization strategies**.

---

# How an Experiment Works

When the user starts an experiment, the execution follows this pipeline:

```text
User configures experiment
            ↓
Next.js frontend sends request
            ↓
Next.js API validates configuration
            ↓
Native Kronos executable is launched
            ↓
C engine selects workload
            ↓
Synchronization mechanism is initialized
            ↓
Worker threads are created
            ↓
Threads execute concurrently
            ↓
Synchronization events are recorded
            ↓
Execution metrics are measured
            ↓
All threads are joined
            ↓
Expected and actual results are calculated
            ↓
Native result is returned
            ↓
Frontend visualizes the execution