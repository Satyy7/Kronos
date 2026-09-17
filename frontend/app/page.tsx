"use client";

import type { ReactNode } from "react";
import { useMemo, useState } from "react";

type SyncType =
  | "NONE"
  | "MUTEX"
  | "SPINLOCK"
  | "RWLOCK"
  | "SEMAPHORE"
  | "CONDVAR"
  | "ATOMIC";

type Workload =
  | "COUNTER"
  | "READERS / WRITERS"
  | "PRODUCER / CONSUMER";

type ExperimentResult = {
  workload: string;
  synchronization: string;
  thread_count: number;
  operations_per_thread: number;
  readers?: number;
  writers?: number;
  producers?: number;
  consumers?: number;
  queue_capacity?: number;
  expected_result: number;
  actual_result: number;
  elapsed_seconds: number;
  throughput: number;
  events_recorded: number;
};

type ExperimentRequest = {
  workload: string;
  synchronization: SyncType;
  operations: number;
  threads?: number;
  readers?: number;
  writers?: number;
  producers?: number;
  consumers?: number;
  queueCapacity?: number;
};

type KronosEvent = {
  type: "event";
  sequence: number;
  timestamp_ns: number;
  thread_id: number;
  event: string;
  value: number;
};

type ThreadState =
  | "READY"
  | "RUNNING"
  | "WAITING"
  | "CRITICAL"
  | "COMPLETE";

const syncTypes: SyncType[] = [
  "NONE",
  "MUTEX",
  "SPINLOCK",
  "RWLOCK",
  "SEMAPHORE",
  "CONDVAR",
  "ATOMIC",
];

const readerWriterSyncTypes: SyncType[] = [
  "MUTEX",
  "SPINLOCK",
  "RWLOCK",
  "SEMAPHORE",
  "CONDVAR",
];

const workloads: Workload[] = [
  "COUNTER",
  "READERS / WRITERS",
  "PRODUCER / CONSUMER",
];

function Icon({
  name,
  size = 18,
}: {
  name: string;
  size?: number;
}) {
  const common = {
    width: size,
    height: size,
    viewBox: "0 0 24 24",
    fill: "none",
    stroke: "currentColor",
    strokeWidth: 1.7,
    strokeLinecap: "round" as const,
    strokeLinejoin: "round" as const,
  };

  const paths: Record<string, ReactNode> = {
    grid: (
      <>
        <rect x="3" y="3" width="7" height="7" rx="1" />
        <rect x="14" y="3" width="7" height="7" rx="1" />
        <rect x="3" y="14" width="7" height="7" rx="1" />
        <rect x="14" y="14" width="7" height="7" rx="1" />
      </>
    ),

    play: <path d="m9 6 10 6-10 6V6Z" />,

    activity: <path d="M3 12h4l2-7 4 14 2-7h6" />,

    layers: (
      <>
        <path d="m12 3 9 5-9 5-9-5 9-5Z" />
        <path d="m3 12 9 5 9-5M3 16l9 5 9-5" />
      </>
    ),

    lock: (
      <>
        <rect x="5" y="10" width="14" height="10" rx="2" />
        <path d="M8 10V7a4 4 0 0 1 8 0v3" />
      </>
    ),

    users: (
      <>
        <circle cx="9" cy="8" r="3" />
        <path d="M3 20c.5-3 2.4-5 6-5s5.5 2 6 5M16 5.5a3 3 0 0 1 0 5.8M18 15c2 .6 3 2.2 3 5" />
      </>
    ),

    clock: (
      <>
        <circle cx="12" cy="12" r="9" />
        <path d="M12 7v5l3 2" />
      </>
    ),

    chevron: <path d="m9 18 6-6-6-6" />,

    settings: (
      <>
        <circle cx="12" cy="12" r="3" />
        <path d="M19.4 15a1.7 1.7 0 0 0 .3 1.9l.1.1-1.7 1.7-.1-.1a1.7 1.7 0 0 0-1.9-.3 1.7 1.7 0 0 0-1 1.5v.2h-2.4v-.2a1.7 1.7 0 0 0-1-1.5 1.7 1.7 0 0 0-1.9.3l-.1.1L8 17l.1-.1a1.7 1.7 0 0 0 .3-1.9 1.7 1.7 0 0 0-1.5-1H6.7v-2.4h.2a1.7 1.7 0 0 0 1.5-1 1.7 1.7 0 0 0-.3-1.9L8 8.6l1.7-1.7.1.1a1.7 1.7 0 0 0 1.9.3 1.7 1.7 0 0 0 1.9-.3l.1-.1 1.7 1.7-.1.1a1.7 1.7 0 0 0-.3 1.9 1.7 1.7 0 0 0 1.5 1h.2V14h-.2a1.7 1.7 0 0 0-1.5 1Z" />
      </>
    ),
  };

  return <svg {...common}>{paths[name]}</svg>;
}

function SectionTitle({
  eyebrow,
  title,
  action,
  onAction,
}: {
  eyebrow: string;
  title: string;
  action?: string;
  onAction?: () => void;
}) {
  return (
    <div className="section-heading">
      <div>
        <div className="eyebrow">{eyebrow}</div>
        <h2>{title}</h2>
      </div>

      {action && (
        <button
          type="button"
          className="text-button"
          onClick={onAction}
        >
          {action}
        </button>
      )}
    </div>
  );
}

function MetricCard({
  label,
  value,
  unit,
  detail,
  icon,
  accent,
}: {
  label: string;
  value: string;
  unit?: string;
  detail: string;
  icon: string;
  accent: string;
}) {
  return (
    <div className="metric-card">
      <div className={`metric-icon ${accent}`}>
        <Icon name={icon} size={17} />
      </div>

      <div className="metric-content">
        <span className="metric-label">{label}</span>

        <div className="metric-value">
          {value}
          {unit && <small>{unit}</small>}
        </div>

        <span className="metric-detail">{detail}</span>
      </div>
    </div>
  );
}

function getRole(
  workload: Workload,
  index: number,
  readers: number,
  producers: number
) {
  if (workload === "READERS / WRITERS") {
    return index < readers ? "READER" : "WRITER";
  }

  if (workload === "PRODUCER / CONSUMER") {
    return index < producers ? "PRODUCER" : "CONSUMER";
  }

  return "WORKER";
}

function getThreadState(
  events: KronosEvent[],
  threadId: number
): ThreadState {
  const threadEvents = events
    .filter((event) => event.thread_id === threadId)
    .sort((a, b) => a.sequence - b.sequence);

  if (threadEvents.length === 0) {
    return "READY";
  }

  const lastEvent =
    threadEvents[threadEvents.length - 1].event;

  switch (lastEvent) {
    case "THREAD_START":
      return "RUNNING";

    case "LOCK_WAIT":
    case "WAIT":
      return "WAITING";

    case "LOCK_ACQUIRE":
      return "CRITICAL";

    case "LOCK_RELEASE":
    case "SIGNAL":
    case "QUEUE_PUSH":
    case "QUEUE_POP":
      return "RUNNING";

    case "THREAD_END":
      return "COMPLETE";

    default:
      return "RUNNING";
  }
}

function getThreadStateClass(state: ThreadState) {
  switch (state) {
    case "RUNNING":
      return "thread-state-running";

    case "WAITING":
      return "thread-state-waiting";

    case "CRITICAL":
      return "thread-state-critical";

    case "COMPLETE":
      return "thread-state-complete";

    case "READY":
    default:
      return "thread-state-ready";
  }
}

function getThreadStateStyle(state: ThreadState) {
  switch (state) {
    case "RUNNING":
      return {
        color: "#31d9ff",
        borderColor: "rgba(49, 217, 255, 0.38)",
        background: "rgba(49, 217, 255, 0.08)",
      };

    case "WAITING":
      return {
        color: "#b59cff",
        borderColor: "rgba(181, 156, 255, 0.38)",
        background: "rgba(181, 156, 255, 0.08)",
      };

    case "CRITICAL":
      return {
        color: "#ffae5c",
        borderColor: "rgba(255, 174, 92, 0.42)",
        background: "rgba(255, 174, 92, 0.08)",
      };

    case "COMPLETE":
      return {
        color: "#53e68b",
        borderColor: "rgba(83, 230, 139, 0.38)",
        background: "rgba(83, 230, 139, 0.08)",
      };

    case "READY":
    default:
      return {
        color: "#8b96a7",
        borderColor: "rgba(139, 150, 167, 0.24)",
        background: "rgba(139, 150, 167, 0.05)",
      };
  }
}

function getEventClass(event: string) {
  switch (event) {
    case "THREAD_START":
      return "event-thread-start";

    case "THREAD_END":
      return "event-thread-end";

    case "LOCK_WAIT":
      return "event-lock-wait";

    case "LOCK_ACQUIRE":
      return "event-lock-acquire";

    case "LOCK_RELEASE":
      return "event-lock-release";

    case "WAIT":
      return "event-wait";

    case "SIGNAL":
      return "event-signal";

    case "QUEUE_PUSH":
      return "event-queue-push";

    case "QUEUE_POP":
      return "event-queue-pop";

    default:
      return "event-default";
  }
}

function getEventStyle(event: string) {
  switch (event) {
    case "THREAD_START":
      return {
        color: "#31d9ff",
        background: "rgba(49, 217, 255, 0.08)",
      };

    case "THREAD_END":
      return {
        color: "#53e68b",
        background: "rgba(83, 230, 139, 0.08)",
      };

    case "LOCK_WAIT":
    case "WAIT":
      return {
        color: "#b59cff",
        background: "rgba(181, 156, 255, 0.08)",
      };

    case "LOCK_ACQUIRE":
      return {
        color: "#ffae5c",
        background: "rgba(255, 174, 92, 0.08)",
      };

    case "LOCK_RELEASE":
      return {
        color: "#7ce7c0",
        background: "rgba(124, 231, 192, 0.07)",
      };

    case "SIGNAL":
      return {
        color: "#f0d76a",
        background: "rgba(240, 215, 106, 0.07)",
      };

    case "QUEUE_PUSH":
      return {
        color: "#53e68b",
        background: "rgba(83, 230, 139, 0.07)",
      };

    case "QUEUE_POP":
      return {
        color: "#6ea8ff",
        background: "rgba(110, 168, 255, 0.07)",
      };

    default:
      return {
        color: "#a8b1bf",
        background: "rgba(168, 177, 191, 0.05)",
      };
  }
}

function formatEventName(event: string) {
  return event
    .replace(/^KRONOS_EVENT_/, "")
    .replace(/_/g, " ");
}

function formatTimestamp(
  timestampNs: number,
  baseNs: number
) {
  if (!baseNs) {
    return "0.000 ms";
  }

  const milliseconds =
    (timestampNs - baseNs) / 1_000_000;

  if (milliseconds < 1) {
    return `${milliseconds.toFixed(3)} ms`;
  }

  return `${milliseconds.toFixed(2)} ms`;
}

function isObject(
  value: unknown
): value is Record<string, unknown> {
  return (
    typeof value === "object" &&
    value !== null
  );
}

function toFiniteNumber(
  value: unknown
): number | null {
  const number = Number(value);

  return Number.isFinite(number)
    ? number
    : null;
}

function normalizeExperimentResult(
  payload: unknown
): ExperimentResult | null {
  if (!isObject(payload)) {
    return null;
  }

  const workload =
    typeof payload.workload === "string"
      ? payload.workload
      : "";

  const synchronization =
    typeof payload.synchronization === "string"
      ? payload.synchronization
      : "";

  const threadCount =
    toFiniteNumber(payload.thread_count);

  const operationsPerThread =
    toFiniteNumber(
      payload.operations_per_thread
    );

  const expectedResult =
    toFiniteNumber(
      payload.expected_result
    );

  const actualResult =
    toFiniteNumber(
      payload.actual_result
    );

  const elapsedSeconds =
    toFiniteNumber(
      payload.elapsed_seconds
    );

  const throughput =
    toFiniteNumber(
      payload.throughput
    );

  const eventsRecorded =
    toFiniteNumber(
      payload.events_recorded
    );

  if (
    threadCount === null ||
    operationsPerThread === null ||
    expectedResult === null ||
    actualResult === null ||
    elapsedSeconds === null ||
    throughput === null ||
    eventsRecorded === null
  ) {
    return null;
  }

  return {
    workload,
    synchronization,
    thread_count: threadCount,
    operations_per_thread:
      operationsPerThread,
    readers:
      toFiniteNumber(payload.readers) ??
      undefined,
    writers:
      toFiniteNumber(payload.writers) ??
      undefined,
    producers:
      toFiniteNumber(payload.producers) ??
      undefined,
    consumers:
      toFiniteNumber(payload.consumers) ??
      undefined,
    queue_capacity:
      toFiniteNumber(
        payload.queue_capacity
      ) ??
      undefined,
    expected_result: expectedResult,
    actual_result: actualResult,
    elapsed_seconds: elapsedSeconds,
    throughput,
    events_recorded: eventsRecorded,
  };
}

export default function Home() {
  const [workload, setWorkload] =
    useState<Workload>("COUNTER");

  const [sync, setSync] =
    useState<SyncType>("MUTEX");

  const [threads, setThreads] =
    useState(4);

  const [readers, setReaders] =
    useState(4);

  const [writers, setWriters] =
    useState(1);

  const [producers, setProducers] =
    useState(2);

  const [consumers, setConsumers] =
    useState(2);

  const [queueCapacity, setQueueCapacity] =
    useState(16);

  const [running, setRunning] =
    useState(false);

  const [result, setResult] =
    useState<ExperimentResult | null>(null);

  const [error, setError] =
    useState<string | null>(null);

  const [events, setEvents] =
    useState<KronosEvent[]>([]);

  const [timelineExpanded, setTimelineExpanded] =
    useState(false);

  const [eventsExpanded, setEventsExpanded] =
    useState(false);

  const [queueExpanded, setQueueExpanded] =
    useState(false);

  const operationsPerThread =
  workload === "COUNTER" && sync === "NONE"
    ? 10000
    : 1000;

  const totalThreads =
    workload === "COUNTER"
      ? threads
      : workload === "READERS / WRITERS"
        ? readers + writers
        : producers + consumers;

  const clearExecutionState = () => {
    setResult(null);
    setError(null);
    setEvents([]);
  };

  const handleWorkloadChange = (
    newWorkload: Workload
  ) => {
    setWorkload(newWorkload);
    clearExecutionState();

    if (newWorkload === "COUNTER") {
      if (!syncTypes.includes(sync)) {
        setSync("MUTEX");
      }
    }

    if (
      newWorkload ===
      "READERS / WRITERS"
    ) {
      if (
        !readerWriterSyncTypes.includes(
          sync
        )
      ) {
        setSync("RWLOCK");
      }
    }

    if (
      newWorkload ===
      "PRODUCER / CONSUMER"
    ) {
      setSync("CONDVAR");
    }
  };

  const handleSyncChange = (
    newSync: SyncType
  ) => {
    if (
      workload ===
      "PRODUCER / CONSUMER"
    ) {
      return;
    }

    if (
      workload ===
        "READERS / WRITERS" &&
      !readerWriterSyncTypes.includes(
        newSync
      )
    ) {
      return;
    }

    setSync(newSync);
    clearExecutionState();
  };

  const syncOptions =
    workload === "COUNTER"
      ? syncTypes
      : workload === "READERS / WRITERS"
        ? readerWriterSyncTypes
        : (["CONDVAR"] as SyncType[]);

  const workloadLabel =
    workload === "COUNTER"
      ? "SHARED COUNTER"
      : workload;

  const runExperiment = async () => {
    if (running) {
      return;
    }

    setRunning(true);
    setError(null);
    setResult(null);
    setEvents([]);

    try {
      const requestBody: ExperimentRequest = {
        workload:
          workload ===
          "READERS / WRITERS"
            ? "READERS_WRITERS"
            : workload ===
                "PRODUCER / CONSUMER"
              ? "PRODUCER_CONSUMER"
              : "COUNTER",

        synchronization: sync,

        operations:
          operationsPerThread,
      };

      if (workload === "COUNTER") {
        requestBody.threads = threads;
      }

      if (
        workload ===
        "READERS / WRITERS"
      ) {
        requestBody.readers = readers;
        requestBody.writers = writers;
      }

      if (
        workload ===
        "PRODUCER / CONSUMER"
      ) {
        requestBody.producers = producers;
        requestBody.consumers = consumers;
        requestBody.queueCapacity =
          queueCapacity;
      }

      const response = await fetch(
        "/api/experiment?stream=1",
        {
          method: "POST",
          headers: {
            Accept:
              "text/event-stream",
            "Content-Type":
              "application/json",
          },
          body: JSON.stringify(
            requestBody
          ),
        }
      );

      if (!response.ok) {
        const text =
          await response.text();

        let message =
          "Experiment failed";

        try {
          const parsed =
            JSON.parse(text);

          if (
            typeof parsed.error ===
            "string"
          ) {
            message =
              parsed.error;
          }
        } catch {
          if (text.trim()) {
            message = text;
          }
        }

        throw new Error(message);
      }

      if (!response.body) {
        throw new Error(
          "Streaming response not available"
        );
      }

      const reader =
        response.body.getReader();

      const decoder =
        new TextDecoder();

      let buffer = "";

      let finalResult:
        | ExperimentResult
        | null = null;

      let streamError:
        | string
        | null = null;

      while (true) {
        const { value, done } =
          await reader.read();

        if (done) {
          break;
        }

        buffer += decoder.decode(
          value,
          {
            stream: true,
          }
        );

        const chunks =
          buffer.split(
            /\r?\n\r?\n/
          );

        buffer =
          chunks.pop() ?? "";

        for (
          const chunk of chunks
        ) {
          const lines =
            chunk.split(/\r?\n/);

          let eventName =
            "message";

          let payloadText = "";

          for (
            const line of lines
          ) {
            if (
              line.startsWith(
                "event:"
              )
            ) {
              eventName =
                line
                  .slice(6)
                  .trim();
            }

            if (
              line.startsWith(
                "data:"
              )
            ) {
              payloadText =
                line
                  .slice(5)
                  .trim();
            }
          }

          if (!payloadText) {
            continue;
          }

          let payload: unknown;

          try {
            payload =
              JSON.parse(
                payloadText
              );
          } catch {
            continue;
          }

          if (
            eventName ===
            "event"
          ) {
            if (
              !isObject(payload)
            ) {
              continue;
            }

            const sequence =
              toFiniteNumber(
                payload.sequence
              );

            const timestampNs =
              toFiniteNumber(
                payload.timestamp_ns
              );

            const threadId =
              toFiniteNumber(
                payload.thread_id
              );

            const eventNameValue =
              typeof payload.event ===
              "string"
                ? payload.event
                : null;

            const value =
              toFiniteNumber(
                payload.value
              );

            if (
              sequence === null ||
              timestampNs === null ||
              threadId === null ||
              eventNameValue ===
                null
            ) {
              continue;
            }

            const kronosEvent:
              KronosEvent = {
              type: "event",
              sequence,
              timestamp_ns:
                timestampNs,
              thread_id:
                threadId,
              event:
                eventNameValue,
              value:
                value ?? 0,
            };

            setEvents(
              (current) => {
                if (
                  current.some(
                    (event) =>
                      event.sequence ===
                      kronosEvent.sequence
                  )
                ) {
                  return current;
                }

                return [
                  ...current,
                  kronosEvent,
                ].sort(
                  (a, b) =>
                    a.sequence -
                    b.sequence
                );
              }
            );

            continue;
          }

          if (
            eventName ===
            "result"
          ) {
            const parsedResult =
              normalizeExperimentResult(
                payload
              );

            if (
              parsedResult !==
              null
            ) {
              finalResult =
                parsedResult;

              setResult(
                parsedResult
              );
            }

            continue;
          }

          if (
            eventName ===
            "error"
          ) {
            if (
              isObject(payload) &&
              typeof payload.message ===
                "string"
            ) {
              streamError =
                payload.message;
            }
          }
        }
      }

      if (streamError) {
        throw new Error(
          streamError
        );
      }

      if (finalResult) {
        setResult(finalResult);
      }
    } catch (err) {
      setError(
        err instanceof Error
          ? err.message
          : "Experiment failed"
      );
    } finally {
      setRunning(false);
    }
  };

  const threadStates =
    useMemo(() => {
      const states =
        new Map<
          number,
          ThreadState
        >();

      for (
        let i = 0;
        i < totalThreads;
        i++
      ) {
        states.set(
          i,
          getThreadState(
            events,
            i
          )
        );
      }

      return states;
    }, [
      events,
      totalThreads,
    ]);

  const threadEventCounts = useMemo(() => {
  const counts: Record<number, number> = {};

  for (const event of events) {
    counts[event.thread_id] =
      (counts[event.thread_id] || 0) + 1;
  }

  return counts;
}, [events]);

  const timelineStart =
    events.length > 0
      ? events[0].timestamp_ns
      : 0;

  const timelineEnd =
    events.length > 0
      ? events[
          events.length - 1
        ].timestamp_ns
      : 0;

  const timelineDuration =
    Math.max(
      1,
      timelineEnd -
        timelineStart
    );

  const timelineData =
    useMemo(() => {
      const output: Record<
        number,
        {
          running: {
            left: number;
            width: number;
          }[];
          waiting: {
            left: number;
            width: number;
          }[];
          critical: {
            left: number;
            width: number;
          }[];
        }
      > = {};

      for (
        let threadId = 0;
        threadId < totalThreads;
        threadId++
      ) {
        output[threadId] = {
          running: [],
          waiting: [],
          critical: [],
        };

        const threadEvents =
          events
            .filter(
              (event) =>
                event.thread_id ===
                threadId
            )
            .sort(
              (a, b) =>
                a.timestamp_ns -
                b.timestamp_ns
            );

        if (
          threadEvents.length ===
          0
        ) {
          continue;
        }

        let runningStart:
          | number
          | null = null;

        let waitingStart:
          | number
          | null = null;

        let criticalStart:
          | number
          | null = null;

        const position =
          (
            timestamp: number
          ) =>
            Math.min(
              100,
              Math.max(
                0,
                ((timestamp -
                  timelineStart) /
                  timelineDuration) *
                  100
              )
            );

        const finishRunning = (
          at: number
        ) => {
          if (
            runningStart !== null
          ) {
            output[
              threadId
            ].running.push({
              left:
                runningStart,
              width: Math.max(
                0.35,
                at -
                  runningStart
              ),
            });

            runningStart =
              null;
          }
        };

        const finishWaiting = (
          at: number
        ) => {
          if (
            waitingStart !== null
          ) {
            output[
              threadId
            ].waiting.push({
              left:
                waitingStart,
              width: Math.max(
                0.35,
                at -
                  waitingStart
              ),
            });

            waitingStart =
              null;
          }
        };

        const finishCritical = (
          at: number
        ) => {
          if (
            criticalStart !== null
          ) {
            output[
              threadId
            ].critical.push({
              left:
                criticalStart,
              width: Math.max(
                0.35,
                at -
                  criticalStart
              ),
            });

            criticalStart =
              null;
          }
        };

        for (
          const event of threadEvents
        ) {
          const p =
            position(
              event.timestamp_ns
            );

          switch (
            event.event
          ) {
            case "THREAD_START":
              runningStart =
                p;
              break;

            case "LOCK_WAIT":
            case "WAIT":
              finishRunning(p);
              waitingStart =
                p;
              break;

            case "LOCK_ACQUIRE":
              finishWaiting(p);

              if (
                runningStart ===
                null
              ) {
                runningStart =
                  p;
              }

              criticalStart =
                p;
              break;

            case "LOCK_RELEASE":
              finishCritical(p);

              if (
                runningStart ===
                null
              ) {
                runningStart =
                  p;
              }

              break;

            case "THREAD_END":
              finishCritical(p);
              finishWaiting(p);
              finishRunning(p);
              break;

            default:
              if (
                runningStart ===
                null
              ) {
                runningStart =
                  p;
              }
          }
        }

        const finalPosition =
          running
            ? 100
            : 100;

        finishCritical(
          finalPosition
        );

        finishWaiting(
          finalPosition
        );

        finishRunning(
          finalPosition
        );
      }

      return output;
    }, [
      events,
      totalThreads,
      timelineStart,
      timelineDuration,
      running,
    ]);

  const queueState =
    useMemo(() => {
      let depth = 0;
      let maximumDepth = 0;
      let pushes = 0;
      let pops = 0;

      const sortedEvents =
        [...events].sort(
          (a, b) =>
            a.sequence -
            b.sequence
        );

      for (
        const event of sortedEvents
      ) {
        if (
          event.event ===
          "QUEUE_PUSH"
        ) {
          depth++;
          pushes++;

          maximumDepth =
            Math.max(
              maximumDepth,
              depth
            );
        }

        if (
          event.event ===
          "QUEUE_POP"
        ) {
          depth =
            Math.max(
              0,
              depth - 1
            );

          pops++;
        }
      }

      return {
        depth,
        maximumDepth,
        pushes,
        pops,
      };
    }, [events]);

  const throughput =
    result &&
    Number.isFinite(
      result.throughput
    )
      ? (
          result.throughput /
          1_000_000
        ).toFixed(2)
      : "—";

  const elapsedValue =
    result &&
    Number.isFinite(
      result.elapsed_seconds
    )
      ? result.elapsed_seconds.toFixed(
          4
        )
      : "—";

  const displayedEventCount =
    result
      ? result.events_recorded
      : events.length;

  const expectedValue =
    result
      ? result.expected_result
      : workload ===
          "PRODUCER / CONSUMER"
        ? producers *
          operationsPerThread
        : workload ===
            "READERS / WRITERS"
          ? writers *
            operationsPerThread
          : threads *
            operationsPerThread;

  const actualValue =
    result
      ? result.actual_result
      : workload ===
          "PRODUCER / CONSUMER"
        ? queueState.pops
        : undefined;

  const recentEvents =
    [...events]
      .sort(
        (a, b) =>
          b.sequence -
          a.sequence
      )
      .slice(0, 18);

  const allEvents =
    [...events].sort(
      (a, b) =>
        b.sequence -
        a.sequence
    );

  const visibleEvents =
    eventsExpanded
      ? allEvents
      : recentEvents;

  const lockRunningStyle =
    running
      ? {
          borderColor:
            "rgba(49, 217, 255, 0.92)",
          boxShadow:
            "0 0 0 10px rgba(49,217,255,.055), 0 0 42px rgba(49,217,255,.42), inset 0 0 42px rgba(49,217,255,.10)",
          transform:
            "scale(1.025)",
        }
      : result
        ? {
            borderColor:
              "rgba(83, 230, 139, 0.85)",
            boxShadow:
              "0 0 0 10px rgba(83,230,139,.035), 0 0 32px rgba(83,230,139,.24), inset 0 0 38px rgba(83,230,139,.075)",
          }
        : undefined;

  const lockCenterStyle =
    running
      ? {
          color:
            "#8eeaff",
          borderColor:
            "rgba(49,217,255,.42)",
        }
      : result
        ? {
            color:
              "#67f1aa",
            borderColor:
              "rgba(83,230,139,.38)",
        }
        : undefined;

  return (
    <main
      className="kronos-shell"
      style={{
        minHeight: "100vh",
        width: "100%",
      }}
    >
      <section
        className="workspace"
        style={{
          width: "100%",
          margin: 0,
        }}
      >
        <header
  className="topbar"
  style={{
    justifyContent: "center",
    paddingTop: "24px",
    paddingBottom: "22px",
  }}
>
  <div
    style={{
      width: "100%",
      display: "flex",
      flexDirection: "column",
      alignItems: "center",
      justifyContent: "center",
      textAlign: "center",
    }}
  >
    <div
      style={{
        display: "flex",
        alignItems: "center",
        justifyContent: "center",
        gap: "14px",
      }}
    >
      <div
        className="brand-mark"
        style={{
          width: "34px",
          height: "34px",
          display: "flex",
          alignItems: "center",
          justifyContent: "center",
          gap: "4px",
        }}
      >
        <span
          style={{
            display: "block",
            width: "5px",
            height: "23px",
            borderRadius: "3px",
          }}
        />
        <span
          style={{
            display: "block",
            width: "5px",
            height: "31px",
            borderRadius: "3px",
          }}
        />
        <span
          style={{
            display: "block",
            width: "5px",
            height: "18px",
            borderRadius: "3px",
          }}
        />
      </div>

      <div
        style={{
          fontSize: "34px",
          fontWeight: 800,
          letterSpacing: "0.18em",
          lineHeight: 1,
          color: "var(--text-primary, #edf4ff)",
        }}
      >
        KRONOS
      </div>
    </div>

    <div
      style={{
        marginTop: "10px",
        fontSize: "11px",
        fontWeight: 400,
        letterSpacing: "0.28em",
        textTransform: "uppercase",
        color: "rgba(180, 192, 208, 0.58)",
      }}
    >
      Execution Control Room
    </div>
  </div>
</header>

        <div className="content">
          <div className="experiment-strip">
            <div className="experiment-title">
              <div className="run-indicator">
                <span />
              </div>

              <div>
                <div className="eyebrow">
                  ACTIVE EXPERIMENT
                </div>

                <h3>
                  {workloadLabel}
                  <span>
                    {" "}
                    ·{" "}
                  </span>
                  {sync}
                </h3>
              </div>
            </div>

            <div className="experiment-actions">
              <button
                className={`run-button ${
                  running
                    ? "running"
                    : ""
                }`}
                onClick={
                  runExperiment
                }
                disabled={running}
              >
                <Icon
                  name={
                    running
                      ? "activity"
                      : "play"
                  }
                  size={17}
                />

                {running
                  ? "RUNNING..."
                  : "RUN EXPERIMENT"}
              </button>

              <button
                className="icon-button"
                title="Settings"
                type="button"
              >
                <Icon
                  name="settings"
                  size={18}
                />
              </button>
            </div>
          </div>

          {error && (
            <div
              style={{
                marginBottom:
                  "16px",
                padding:
                  "12px 16px",
                border:
                  "1px solid rgba(255,80,80,.35)",
                borderRadius:
                  "8px",
                color:
                  "#ff8080",
                background:
                  "rgba(255,80,80,.06)",
                fontSize:
                  "13px",
              }}
            >
              {error}
            </div>
          )}

          {(result || running) && (
            <div
              style={{
                marginBottom:
                  "16px",
                display:
                  "flex",
                gap: "28px",
                flexWrap:
                  "wrap",
                padding:
                  "14px 18px",
                border:
                  "1px solid rgba(120,220,180,.2)",
                borderRadius:
                  "8px",
                background:
                  "rgba(120,220,180,.035)",
                fontSize:
                  "13px",
              }}
            >
              <span>
                EXPECTED{" "}
                <strong>
                  {expectedValue.toLocaleString()}
                </strong>
              </span>

              <span>
                ACTUAL{" "}
                <strong>
                  {actualValue !==
                  undefined
                    ? actualValue.toLocaleString()
                    : running
                      ? "EXECUTING"
                      : "—"}
                </strong>
              </span>

              <span>
                EVENTS{" "}
                <strong>
                  {displayedEventCount.toLocaleString()}
                </strong>
              </span>

              <span>
                STATUS{" "}
                <strong>
                  {running
                    ? "EXECUTING"
                    : result
                      ? "COMPLETE"
                      : "READY"}
                </strong>
              </span>
            </div>
          )}

          <section className="configuration-panel panel">
            <div className="panel-heading">
              <div>
                <div className="eyebrow">
                  EXPERIMENT CONFIGURATION
                </div>

                <h2>
                  Execution Parameters
                </h2>
              </div>

              <div className="config-id">
                EXP-0001
              </div>
            </div>

            <div className="config-grid">
              <label className="field">
                <span>WORKLOAD</span>

                <select
                  value={workload}
                  disabled={running}
                  onChange={(e) =>
                    handleWorkloadChange(
                      e.target
                        .value as Workload
                    )
                  }
                >
                  {workloads.map(
                    (item) => (
                      <option
                        key={item}
                        value={item}
                      >
                        {item}
                      </option>
                    )
                  )}
                </select>
              </label>

              <label className="field">
                <span>
                  SYNCHRONIZATION
                </span>

                <select
                  value={sync}
                  disabled={
                    running ||
                    workload ===
                      "PRODUCER / CONSUMER"
                  }
                  onChange={(e) =>
                    handleSyncChange(
                      e.target
                        .value as SyncType
                    )
                  }
                >
                  {syncOptions.map(
                    (item) => (
                      <option
                        key={item}
                        value={item}
                      >
                        {item}
                      </option>
                    )
                  )}
                </select>
              </label>

              {workload ===
                "COUNTER" && (
                <>
                  <label className="field">
                    <span>
                      WORKER THREADS
                    </span>

                    <div className="number-control">
                      <button
                        type="button"
                        disabled={
                          running ||
                          threads <= 1
                        }
                        onClick={() => {
                          setThreads(
                            Math.max(
                              1,
                              threads -
                                1
                            )
                          );
                          clearExecutionState();
                        }}
                      >
                        −
                      </button>

                      <strong>
                        {threads}
                      </strong>

                      <button
                        type="button"
                        disabled={
                          running ||
                          threads >= 8
                        }
                        onClick={() => {
                          setThreads(
                            Math.min(
                              8,
                              threads +
                                1
                            )
                          );
                          clearExecutionState();
                        }}
                      >
                        +
                      </button>
                    </div>
                  </label>

                  <label className="field">
                    <span>
                      OPERATIONS /
                      THREAD
                    </span>

                    <input
                      value={operationsPerThread.toLocaleString()}
                      readOnly
                    />
                  </label>
                </>
              )}

              {workload ===
                "READERS / WRITERS" && (
                <>
                  <label className="field">
                    <span>
                      READER THREADS
                    </span>

                    <div className="number-control">
                      <button
                        type="button"
                        disabled={
                          running ||
                          readers <= 1
                        }
                        onClick={() => {
                          setReaders(
                            Math.max(
                              1,
                              readers -
                                1
                            )
                          );
                          clearExecutionState();
                        }}
                      >
                        −
                      </button>

                      <strong>
                        {readers}
                      </strong>

                      <button
                        type="button"
                        disabled={
                          running ||
                          readers +
                            writers >=
                            8
                        }
                        onClick={() => {
                          setReaders(
                            Math.min(
                              8 -
                                writers,
                              readers +
                                1
                            )
                          );
                          clearExecutionState();
                        }}
                      >
                        +
                      </button>
                    </div>
                  </label>

                  <label className="field">
                    <span>
                      WRITER THREADS
                    </span>

                    <div className="number-control">
                      <button
                        type="button"
                        disabled={
                          running ||
                          writers <= 1
                        }
                        onClick={() => {
                          setWriters(
                            Math.max(
                              1,
                              writers -
                                1
                            )
                          );
                          clearExecutionState();
                        }}
                      >
                        −
                      </button>

                      <strong>
                        {writers}
                      </strong>

                      <button
                        type="button"
                        disabled={
                          running ||
                          readers +
                            writers >=
                            8
                        }
                        onClick={() => {
                          setWriters(
                            Math.min(
                              8 -
                                readers,
                              writers +
                                1
                            )
                          );
                          clearExecutionState();
                        }}
                      >
                        +
                      </button>
                    </div>
                  </label>

                  <label className="field">
                    <span>
                      TOTAL THREADS
                    </span>

                    <input
                      value={`${totalThreads} threads`}
                      readOnly
                    />
                  </label>

                  <label className="field">
                    <span>
                      OPERATIONS /
                      THREAD
                    </span>

                    <input
                      value={operationsPerThread.toLocaleString()}
                      readOnly
                    />
                  </label>
                </>
              )}

              {workload ===
                "PRODUCER / CONSUMER" && (
                <>
                  <label className="field">
                    <span>
                      PRODUCER THREADS
                    </span>

                    <div className="number-control">
                      <button
                        type="button"
                        disabled={
                          running ||
                          producers <= 1
                        }
                        onClick={() => {
                          setProducers(
                            Math.max(
                              1,
                              producers -
                                1
                            )
                          );
                          clearExecutionState();
                        }}
                      >
                        −
                      </button>

                      <strong>
                        {producers}
                      </strong>

                      <button
                        type="button"
                        disabled={
                          running ||
                          producers +
                            consumers >=
                            8
                        }
                        onClick={() => {
                          setProducers(
                            Math.min(
                              8 -
                                consumers,
                              producers +
                                1
                            )
                          );
                          clearExecutionState();
                        }}
                      >
                        +
                      </button>
                    </div>
                  </label>

                  <label className="field">
                    <span>
                      CONSUMER THREADS
                    </span>

                    <div className="number-control">
                      <button
                        type="button"
                        disabled={
                          running ||
                          consumers <= 1
                        }
                        onClick={() => {
                          setConsumers(
                            Math.max(
                              1,
                              consumers -
                                1
                            )
                          );
                          clearExecutionState();
                        }}
                      >
                        −
                      </button>

                      <strong>
                        {consumers}
                      </strong>

                      <button
                        type="button"
                        disabled={
                          running ||
                          producers +
                            consumers >=
                            8
                        }
                        onClick={() => {
                          setConsumers(
                            Math.min(
                              8 -
                                producers,
                              consumers +
                                1
                            )
                          );
                          clearExecutionState();
                        }}
                      >
                        +
                      </button>
                    </div>
                  </label>

                  <label className="field">
                    <span>
                      QUEUE CAPACITY
                    </span>

                    <div className="number-control">
                      <button
                        type="button"
                        disabled={
                          running ||
                          queueCapacity <=
                            1
                        }
                        onClick={() => {
                          setQueueCapacity(
                            Math.max(
                              1,
                              queueCapacity -
                                1
                            )
                          );
                          clearExecutionState();
                        }}
                      >
                        −
                      </button>

                      <strong>
                        {queueCapacity}
                      </strong>

                      <button
                        type="button"
                        disabled={
                          running ||
                          queueCapacity >=
                            128
                        }
                        onClick={() => {
                          setQueueCapacity(
                            Math.min(
                              128,
                              queueCapacity +
                                1
                            )
                          );
                          clearExecutionState();
                        }}
                      >
                        +
                      </button>
                    </div>
                  </label>

                  <label className="field">
                    <span>
                      OPERATIONS /
                      PRODUCER
                    </span>

                    <input
                      value={operationsPerThread.toLocaleString()}
                      readOnly
                    />
                  </label>

                  <label className="field">
                    <span>
                      TOTAL THREADS
                    </span>

                    <input
                      value={`${totalThreads} threads`}
                      readOnly
                    />
                  </label>
                </>
              )}
            </div>
          </section>

          <section
            className="metrics-grid"
            style={{
              display: "grid",
              gridTemplateColumns:
                "repeat(4, minmax(0, 1fr))",
              gap: "12px",
            }}
          >
            <MetricCard
              label="THREADS"
              value={String(
                result?.thread_count ??
                  totalThreads
              )}
              unit="workers"
              detail={`${totalThreads} active execution contexts`}
              icon="users"
              accent="cyan"
            />

            <MetricCard
              label="THROUGHPUT"
              value={throughput}
              unit="M ops/s"
              detail={
                result
                  ? "Measured execution throughput"
                  : running
                    ? "Live execution"
                    : "No experiment executed"
              }
              icon="activity"
              accent="orange"
            />

            <MetricCard
              label="EVENTS"
              value={displayedEventCount.toLocaleString()}
              detail={
                running
                  ? "Live C-engine instrumentation"
                  : result
                    ? "Complete recorded event count"
                    : "No experiment executed"
              }
              icon="layers"
              accent="green"
            />

            <MetricCard
              label="ELAPSED TIME"
              value={elapsedValue}
              unit="s"
              detail={
                result
                  ? "Native CLOCK_MONOTONIC measurement"
                  : running
                    ? "Execution in progress"
                    : "No experiment executed"
              }
              icon="clock"
              accent="violet"
            />
          </section>

          <div
            className="main-grid"
            style={{
              alignItems:
                "start",
            }}
          >
            <section
              className="panel timeline-panel"
              style={
                timelineExpanded
                  ? {
                      position:
                        "fixed",
                      inset:
                        "24px",
                      zIndex:
                        1000,
                      overflow:
                        "auto",
                    }
                  : undefined
              }
            >
              <SectionTitle
                eyebrow="EXECUTION TRACE"
                title="Thread Timeline"
                action={
                  timelineExpanded
                    ? "CLOSE"
                    : "EXPAND"
                }
                onAction={() =>
                  setTimelineExpanded(
                    (value) =>
                      !value
                  )
                }
              />

              <div className="timeline-meta">
                <span>
                  <i className="legend-running" />
                  RUNNING
                </span>

                <span>
                  <i className="legend-waiting" />
                  WAITING
                </span>

                <span>
                  <i className="legend-critical" />
                  CRITICAL SECTION
                </span>

                <span className="trace-time">
                  {events.length > 0
                    ? `${events.length.toLocaleString()} recorded events`
                    : "Native event timeline"}
                </span>
              </div>

              {events.length ===
              0 ? (
                <div
                  style={{
                    padding:
                      "48px 20px",
                    textAlign:
                      "center",
                    opacity: 0.55,
                    fontSize:
                      "13px",
                  }}
                >
                  {running
                    ? "Waiting for native execution events..."
                    : "Run an experiment to build the real thread timeline."}
                </div>
              ) : (
                <div
                  className="timeline"
                  style={{
                    minWidth:
                      timelineExpanded
                        ? "900px"
                        : undefined,
                  }}
                >
                  <div className="timeline-axis">
                    <span>0%</span>
                    <span>20</span>
                    <span>40</span>
                    <span>60</span>
                    <span>80</span>
                    <span>100%</span>
                  </div>

                  {Array.from({
                    length:
                      totalThreads,
                  }).map(
                    (_, index) => {
                      const row =
                        timelineData[
                          index
                        ];

                      const state =
                        threadStates.get(
                          index
                        ) ??
                        "READY";

                      const stateStyle =
                        getThreadStateStyle(
                          state
                        );

                      return (
                        <div
                          className="thread-row"
                          key={index}
                        >
                          <div className="thread-label">
                            <span className="thread-id">
                              T
                              {index}
                            </span>

                            <span>
                              {getRole(
                                workload,
                                index,
                                readers,
                                producers
                              )}
                            </span>

                            <span
                              style={{
                                marginLeft:
                                  "auto",
                                fontSize:
                                  "9px",
                                color:
                                  stateStyle.color,
                              }}
                            >
                              {state}
                            </span>
                          </div>

                          <div
                            className="thread-track"
                            style={{
                              position:
                                "relative",
                              minHeight:
                                "22px",
                            }}
                          >
                            {row?.running.map(
                              (
                                segment,
                                i
                              ) => (
                                <div
                                  key={`r-${i}`}
                                  className="execution-segment"
                                  style={{
                                    position:
                                      "absolute",
                                    left: `${segment.left}%`,
                                    width: `${segment.width}%`,
                                  }}
                                />
                              )
                            )}

                            {row?.waiting.map(
                              (
                                segment,
                                i
                              ) => (
                                <div
                                  key={`w-${i}`}
                                  className="execution-segment secondary"
                                  style={{
                                    position:
                                      "absolute",
                                    left: `${segment.left}%`,
                                    width: `${segment.width}%`,
                                  }}
                                />
                              )
                            )}

                            {row?.critical.map(
                              (
                                segment,
                                i
                              ) => (
                                <div
                                  key={`c-${i}`}
                                  className="critical-segment"
                                  style={{
                                    position:
                                      "absolute",
                                    left: `${segment.left}%`,
                                    width: `${segment.width}%`,
                                  }}
                                />
                              )
                            )}
                          </div>
                        </div>
                      );
                    }
                  )}
                </div>
              )}
            </section>

            <section className="panel sync-panel">
  <SectionTitle
    eyebrow="SYNCHRONIZATION"
    title="Lock State"
    action="DETAILS"
  />

  <div className="lock-visual">
    <div
      className={`lock-ring ${
        running
          ? "sync-executing"
          : result
            ? "sync-success"
            : ""
      }`}
      style={lockRunningStyle}
    >
      <div
        className="lock-center"
        style={lockCenterStyle}
      >
        <Icon
          name="lock"
          size={27}
        />

        <strong>
          {sync}
        </strong>

        <span>
          {running
            ? "EXECUTING"
            : result
              ? "COMPLETE"
              : "READY"}
        </span>
      </div>
    </div>

    <div className="lock-stat">
      <span>
        THREADS
      </span>

      <strong>
        {totalThreads}
      </strong>
    </div>

    <div className="lock-stat">
      <span>
        EVENTS
      </span>

      <strong>
        {displayedEventCount.toLocaleString()}
      </strong>
    </div>
  </div>

  <div className="waiter-list">
    <div className="waiter-header">
      <span>
        THREAD
      </span>

      <span>
        EVENTS
      </span>

      <span>
        ROLE
      </span>
    </div>

    <div
      className="thread-state-grid"
      style={{
        display: "grid",
        gridTemplateColumns:
          "repeat(2, minmax(0, 1fr))",
        gap: "8px",
      }}
    >
      {Array.from({
        length: totalThreads,
      }).map((_, index) => {
        const role = getRole(
          workload,
          index,
          readers,
          producers
        );

        const threadEventCount =
          events.reduce(
            (count, event) =>
              event.thread_id === index
                ? count + 1
                : count,
            0
          );

        return (
          <div
            className="thread-state-card"
            key={`thread-${index}`}
            style={{
              display: "grid",
              gridTemplateColumns:
                "42px minmax(0, 1fr) auto",
              alignItems: "center",
              gap: "8px",
              padding: "9px 10px",
              border:
                "1px solid rgba(255,255,255,.07)",
              borderRadius: "7px",
              background:
                "rgba(255,255,255,.018)",
            }}
          >
            <span
              className="thread-id"
              style={{
                fontSize: "11px",
              }}
            >
              T{index}
            </span>

            <div
              style={{
                display: "inline-flex",
                alignItems: "center",
                gap: "6px",
                width: "fit-content",
                padding: "4px 7px",
                border:
                  "1px solid rgba(255,255,255,.08)",
                borderRadius: "5px",
                color:
                  "rgba(210,220,235,.85)",
                background:
                  "rgba(255,255,255,.025)",
                fontSize: "9px",
                fontWeight: 700,
                letterSpacing: "0.04em",
              }}
            >
              <span
                style={{
                  width: "5px",
                  height: "5px",
                  minWidth: "5px",
                  borderRadius: "50%",
                  background:
                    running
                      ? "#22d3ee"
                      : "rgba(180,192,208,.55)",
                  boxShadow:
                    running
                      ? "0 0 8px rgba(34,211,238,.65)"
                      : "none",
                }}
              />

              {threadEventCount.toLocaleString()} EVENTS
            </div>

            <span
              className="thread-role"
              style={{
                fontSize: "9px",
                opacity: 0.65,
                whiteSpace: "nowrap",
              }}
            >
              {role}
            </span>
          </div>
        );
      })}
    </div>
  </div>
</section>
          </div>

          <section
            className="panel events-panel"
            style={{
              width: "100%",
              gridColumn:
                "1 / -1",
            }}
          >
            <SectionTitle
              eyebrow="INSTRUMENTATION"
              title="Event Stream"
              action={
                eventsExpanded
                  ? "CLOSE"
                  : "VIEW ALL"
              }
              onAction={() =>
                setEventsExpanded(
                  (value) =>
                    !value
                )
              }
            />

            <div
              className="event-table"
              style={{
                width:
                  "100%",
                maxHeight:
                  eventsExpanded
                    ? "65vh"
                    : "390px",
                overflowY:
                  "auto",
                overflowX:
                  "hidden",
              }}
            >
              {visibleEvents.length ===
              0 ? (
                <div
                  style={{
                    padding:
                      "38px 20px",
                    textAlign:
                      "center",
                    opacity:
                      0.55,
                    fontSize:
                      "13px",
                  }}
                >
                  {running
                    ? "Waiting for native execution events..."
                    : "Run an experiment to capture real C-engine events."}
                </div>
              ) : (
                <>
                  <div
                    style={{
                      display:
                        "grid",
                      gridTemplateColumns:
                        "90px 62px minmax(170px, 1fr) 100px",
                      gap:
                        "12px",
                      alignItems:
                        "center",
                      padding:
                        "9px 10px",
                      position:
                        "sticky",
                      top: 0,
                      zIndex: 5,
                      background:
                        "var(--background, #0b0f14)",
                      borderBottom:
                        "1px solid rgba(255,255,255,.07)",
                      fontSize:
                        "9px",
                      letterSpacing:
                        "0.08em",
                      opacity:
                        0.6,
                    }}
                  >
                    <span>
                      TIME
                    </span>

                    <span>
                      THREAD
                    </span>

                    <span>
                      EVENT
                    </span>

                    <span
                      style={{
                        textAlign:
                          "right",
                      }}
                    >
                      VALUE
                    </span>
                  </div>

                  {visibleEvents.map(
                    (event) => {
                      const eventStyle =
                        getEventStyle(
                          event.event
                        );

                      return (
                        <div
                          key={`${event.sequence}-${event.thread_id}`}
                          style={{
                            display:
                              "grid",
                            gridTemplateColumns:
                              "90px 62px minmax(170px, 1fr) 100px",
                            gap:
                              "12px",
                            alignItems:
                              "center",
                            padding:
                              "8px 10px",
                            borderBottom:
                              "1px solid rgba(255,255,255,.045)",
                            fontSize:
                              "11px",
                          }}
                        >
                          <span
                            className="event-time"
                            style={{
                              whiteSpace:
                                "nowrap",
                              fontVariantNumeric:
                                "tabular-nums",
                              opacity:
                                0.65,
                            }}
                          >
                            {formatTimestamp(
                              event.timestamp_ns,
                              timelineStart
                            )}
                          </span>

                          <span
                            className="thread-id"
                            style={{
                              fontSize:
                                "10px",
                            }}
                          >
                            T
                            {
                              event.thread_id
                            }
                          </span>

                          <span
                            className={`event-type ${getEventClass(
                              event.event
                            )}`}
                            style={{
                              display:
                                "inline-flex",
                              alignItems:
                                "center",
                              width:
                                "fit-content",
                              maxWidth:
                                "100%",
                              padding:
                                "4px 8px",
                              borderRadius:
                                "4px",
                              color:
                                eventStyle.color,
                              background:
                                eventStyle.background,
                              fontSize:
                                "9px",
                              fontWeight:
                                700,
                              letterSpacing:
                                "0.05em",
                              overflow:
                                "hidden",
                              textOverflow:
                                "ellipsis",
                              whiteSpace:
                                "nowrap",
                            }}
                          >
                            {formatEventName(
                              event.event
                            )}
                          </span>

                          <span
                            style={{
                              textAlign:
                                "right",
                              fontVariantNumeric:
                                "tabular-nums",
                              color:
                                event.value !==
                                0
                                  ? "#d6dde8"
                                  : "transparent",
                              minHeight:
                                "14px",
                            }}
                          >
                            {event.value !==
                            0
                              ? event.value.toLocaleString()
                              : ""}
                          </span>
                        </div>
                      );
                    }
                  )}
                </>
              )}
            </div>
          </section>

          {workload ===
            "PRODUCER / CONSUMER" && (
            <section
              className="panel queue-panel"
              style={
                queueExpanded
                  ? {
                      position:
                        "fixed",
                      inset:
                        "24px",
                      zIndex:
                        1000,
                      overflow:
                        "auto",
                    }
                  : undefined
              }
            >
              <SectionTitle
                eyebrow="WORKLOAD STATE"
                title="Bounded Queue"
                action={
                  queueExpanded
                    ? "CLOSE"
                    : "INSPECT"
                }
                onAction={() =>
                  setQueueExpanded(
                    (value) =>
                      !value
                  )
                }
              />

              <div className="queue-layout">
                <div className="queue-capacity">
                  <span>
                    CAPACITY
                  </span>

                  <strong>
                    {result?.queue_capacity ??
                      queueCapacity}
                  </strong>

                  <small>
                    slots
                  </small>
                </div>

                <div
                  className="queue-track"
                  style={{
                    position:
                      "relative",
                    overflow:
                      "hidden",
                  }}
                >
                  {Array.from({
                    length:
                      Math.min(
                        result?.queue_capacity ??
                          queueCapacity,
                        32
                      ),
                  }).map(
                    (_, index) => {
                      const occupied =
                        index <
                        Math.min(
                          queueState.depth,
                          32
                        );

                      return (
                        <div
                          className={`queue-slot ${
                            occupied
                              ? "filled"
                              : ""
                          }`}
                          key={index}
                          style={{
                            opacity:
                              occupied
                                ? 1
                                : 0.45,
                          }}
                        />
                      );
                    }
                  )}
                </div>

                <div className="queue-stats">
                  <div>
                    <span>
                      EXPECTED
                    </span>

                    <strong>
                      {result
                        ? result.expected_result.toLocaleString()
                        : (
                            producers *
                            operationsPerThread
                          ).toLocaleString()}
                    </strong>
                  </div>

                  <div>
                    <span>
                      CONSUMED
                    </span>

                    <strong>
                      {result
                        ? result.actual_result.toLocaleString()
                        : queueState.pops.toLocaleString()}
                    </strong>
                  </div>

                  <div>
                    <span>
                      DEPTH
                    </span>

                    <strong>
                      {queueState.depth}
                    </strong>
                  </div>

                  <div>
                    <span>
                      MAX DEPTH
                    </span>

                    <strong>
                      {queueState.maximumDepth}
                    </strong>
                  </div>
                </div>
              </div>
            </section>
          )}

          <footer className="footer">
            <span>
              KRONOS REAL-TIME MULTICORE
              CONCURRENCY LAB
            </span>

            <span>
              C11 · PTHREADS · POSIX ·
              REAL EXECUTION
            </span>
          </footer>
        </div>
      </section>
    </main>
  );
}