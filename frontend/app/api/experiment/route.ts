
import { NextRequest } from "next/server";
import { spawn, spawnSync } from "child_process";
import path from "path";

interface ExperimentRequest {
  workload: string;
  synchronization: string;
  threads?: number;
  operations: number;
  readers?: number;
  writers?: number;
  producers?: number;
  consumers?: number;
  queueCapacity?: number;
}

function validateRequest(
  body: ExperimentRequest
): string | null {
  if (!body.workload) {
    return "Workload is required";
  }

  if (!body.synchronization) {
    return "Synchronization is required";
  }

  const maxOperations =
  body.workload === "COUNTER" && body.synchronization === "NONE"
    ? 10000
    : 1000;

if (
  !Number.isInteger(body.operations) ||
  body.operations <= 0 ||
  body.operations > maxOperations
) {
  return `Operations must be an integer between 1 and ${maxOperations}`;
}

  if (body.workload === "COUNTER") {
    if (
      !Number.isInteger(body.threads) ||
      (body.threads ?? 0) <= 0
    ) {
      return "Thread count must be a positive integer";
    }
  }

  if (body.workload === "READERS_WRITERS") {
    if (
      !Number.isInteger(body.readers) ||
      (body.readers ?? 0) <= 0
    ) {
      return "Reader count must be a positive integer";
    }

    if (
      !Number.isInteger(body.writers) ||
      (body.writers ?? 0) <= 0
    ) {
      return "Writer count must be a positive integer";
    }
  }

  if (body.workload === "PRODUCER_CONSUMER") {
    if (
      !Number.isInteger(body.producers) ||
      (body.producers ?? 0) <= 0
    ) {
      return "Producer count must be a positive integer";
    }

    if (
      !Number.isInteger(body.consumers) ||
      (body.consumers ?? 0) <= 0
    ) {
      return "Consumer count must be a positive integer";
    }

    if (
      !Number.isInteger(body.queueCapacity) ||
      (body.queueCapacity ?? 0) <= 0
    ) {
      return "Queue capacity must be a positive integer";
    }
  }

  return null;
}

function buildArguments(
  body: ExperimentRequest
): string[] {
  const args: string[] = [
    "--workload",
    body.workload,
    "--sync",
    body.synchronization,
    "--operations",
    String(body.operations),
  ];

  if (body.workload === "COUNTER") {
    args.push(
      "--threads",
      String(body.threads ?? 4)
    );
  }

  if (body.workload === "READERS_WRITERS") {
    args.push(
      "--readers",
      String(body.readers ?? 2),
      "--writers",
      String(body.writers ?? 2)
    );
  }

  if (body.workload === "PRODUCER_CONSUMER") {
    args.push(
      "--producers",
      String(body.producers ?? 2),
      "--consumers",
      String(body.consumers ?? 2),
      "--queue-capacity",
      String(body.queueCapacity ?? 2)
    );
  }

  args.push(
    "--json",
    "--live"
  );

  return args;
}

function createSSEMessage(
  event: string,
  data: unknown
): string {
  return (
    `event: ${event}\n` +
    `data: ${JSON.stringify(data)}\n\n`
  );
}

function runKronosOnce(
  args: string[]
): unknown {
  const executable =
    path.resolve(
      process.cwd(),
      "..",
      "kronos"
    );

  const workingDirectory =
    path.resolve(
      process.cwd(),
      ".."
    );

  const result = spawnSync(
    executable,
    args,
    {
      cwd: workingDirectory,
      encoding: "utf8",
    }
  );

  if (result.error) {
    throw result.error;
  }

  if (result.status !== 0) {
    throw new Error(
      (result.stderr || "").trim() ||
        `Kronos exited with code ${result.status}`
    );
  }

  if (!result.stdout.trim()) {
    throw new Error(
      "Kronos returned no output"
    );
  }

  try {
    return JSON.parse(
      result.stdout
    );
  } catch {
    throw new Error(
      `Invalid JSON returned by Kronos: ${result.stdout}`
    );
  }
}

export async function POST(
  request: NextRequest
) {
  let body: ExperimentRequest;

  try {
    body =
      (await request.json()) as ExperimentRequest;
  } catch {
    return new Response(
      JSON.stringify({
        error: "Invalid JSON request body",
      }),
      {
        status: 400,
        headers: {
          "Content-Type":
            "application/json",
        },
      }
    );
  }

  const validationError =
    validateRequest(body);

  if (validationError !== null) {
    return new Response(
      JSON.stringify({
        error: validationError,
      }),
      {
        status: 400,
        headers: {
          "Content-Type":
            "application/json",
        },
      }
    );
  }

  const args =
    buildArguments(body);

  const wantsStream =
    request.nextUrl.searchParams.get(
      "stream"
    ) === "1" ||
    request.headers
      .get("accept")
      ?.includes("text/event-stream") === true;

  if (!wantsStream) {
    try {
      const result =
        runKronosOnce(args);

      return Response.json(
        result,
        {
          status: 200,
        }
      );
    } catch (error) {
      console.error(
        "Kronos experiment failed:",
        error
      );

      return Response.json(
        {
          error:
            error instanceof Error
              ? error.message
              : "Experiment failed",
        },
        {
          status: 500,
        }
      );
    }
  }

  const encoder =
    new TextEncoder();

  const stream =
    new ReadableStream<Uint8Array>({
      start(controller) {
        const executable =
          path.resolve(
            process.cwd(),
            "..",
            "kronos"
          );

        const workingDirectory =
          path.resolve(
            process.cwd(),
            ".."
          );

        const kronosProcess =
          spawn(
            executable,
            args,
            {
              cwd: workingDirectory,
              stdio: [
                "ignore",
                "pipe",
                "pipe",
              ],
            }
          );

        let stdoutBuffer = "";
        let stderrBuffer = "";
        let closed = false;

        const send = (
          event: string,
          data: unknown
        ) => {
          if (closed) {
            return;
          }

          controller.enqueue(
            encoder.encode(
              createSSEMessage(
                event,
                data
              )
            )
          );
        };

        send(
          "experiment_start",
          {
            workload:
              body.workload,
            synchronization:
              body.synchronization,
            operations:
              body.operations,
            threads:
              body.threads,
            readers:
              body.readers,
            writers:
              body.writers,
            producers:
              body.producers,
            consumers:
              body.consumers,
            queueCapacity:
              body.queueCapacity,
          }
        );

        kronosProcess.stdout.on(
  "data",
  (chunk: Buffer) => {
    stdoutBuffer += chunk.toString();
  }
);

        kronosProcess.stderr.on(
          "data",
          (chunk: Buffer) => {
            stderrBuffer +=
              chunk.toString();

            let newlineIndex =
              stderrBuffer.indexOf(
                "\n"
              );

            while (
              newlineIndex !== -1
            ) {
              const line =
                stderrBuffer
                  .slice(
                    0,
                    newlineIndex
                  )
                  .trim();

              stderrBuffer =
                stderrBuffer.slice(
                  newlineIndex + 1
                );

              if (
                line.length === 0
              ) {
                newlineIndex =
                  stderrBuffer.indexOf(
                    "\n"
                  );

                continue;
              }

              try {
                const parsed =
                  JSON.parse(line);

                if (
                  parsed.type ===
                  "event"
                ) {
                  send(
                    "event",
                    parsed
                  );
                } else {
                  send(
                    "telemetry",
                    parsed
                  );
                }
              } catch {
                send(
                  "stderr",
                  {
                    message: line,
                  }
                );
              }

              newlineIndex =
                stderrBuffer.indexOf(
                  "\n"
                );
            }
          }
        );

        kronosProcess.on(
          "error",
          (error: Error) => {
            if (closed) {
              return;
            }

            send(
              "error",
              {
                message:
                  error.message,
              }
            );

            closed = true;

            controller.close();
          }
        );

        kronosProcess.on(
          "close",
          (code: number | null) => {
            if (closed) {
              return;
            }

            if (
              stdoutBuffer.trim()
                .length > 0
            ) {
              try {
                const result =
                  JSON.parse(
                    stdoutBuffer.trim()
                  );

                send(
                  "result",
                  result
                );
              } catch {
                send(
                  "stdout",
                  {
                    message:
                      stdoutBuffer.trim(),
                  }
                );
              }
            }

            if (
              stderrBuffer.trim()
                .length > 0
            ) {
              try {
                const parsed =
                  JSON.parse(
                    stderrBuffer.trim()
                  );

                if (
                  parsed.type ===
                  "event"
                ) {
                  send(
                    "event",
                    parsed
                  );
                } else {
                  send(
                    "telemetry",
                    parsed
                  );
                }
              } catch {
                send(
                  "stderr",
                  {
                    message:
                      stderrBuffer.trim(),
                  }
                );
              }
            }

            send(
              "experiment_end",
              {
                exit_code: code,
              }
            );

            closed = true;

            controller.close();
          }
        );

        request.signal.addEventListener(
          "abort",
          () => {
            if (closed) {
              return;
            }

            closed = true;

            try {
              kronosProcess.kill(
                "SIGTERM"
              );
            } catch {
            }

            controller.close();
          }
        );
      },

      cancel() {
      },
    });

  return new Response(
    stream,
    {
      status: 200,
      headers: {
        "Content-Type":
          "text/event-stream",
        "Cache-Control":
          "no-cache, no-transform",
        Connection:
          "keep-alive",
        "X-Accel-Buffering":
          "no",
      },
    }
  );
}

