# Performance Tests

[Test index](../README.md)

Performance tests measure throughput and latency for explicitly
complexity-sensitive paths. They validate their results, but they do not impose
machine-independent speed thresholds.

## Build and run

Use a Release build on a quiet machine:

```sh
cmake -S . -B build-perf -DBUILD_TESTING=ON \
  -DLIBGS_BUILD_PERFORMANCE_TESTS=ON \
  -DLIBGS_BUILD_HTTP=ON \
  -DLIBGS_BUILD_WEBSOCKET=ON \
  -DLIBGS_BUILD_UTILITIES=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-perf --parallel
ctest --test-dir build-perf -L performance -V
```

Performance entries run serially at the CTest level. Verbose output is useful
because the measurements are printed by each executable.

## Measurements

| CTest entry | Coverage |
| --- | --- |
| `libgs.performance.lock-free-queue` | SPSC and MPMC linked/circular queue throughput |
| `libgs.performance.core` | Text, URL, hash, and input-size scaling |
| `libgs.performance.coro` | Coroutine mutex, semaphore, condition, and waiter overhead |
| `libgs.performance.http` | HTTP loopback request throughput and payload shapes |
| `libgs.performance.http-protocol` | HTTP parser/generator and optional gzip throughput |
| `libgs.performance.websocket` | Frame codec and loopback message throughput |
| `libgs.performance.logger` | Formatting, dispatch, and file output |
| `libgs.performance.signal-slot` | Synchronous/asynchronous emission and connection operations |
| `libgs.performance.sbus` | Publish and end-to-end local soft-bus fanout |

Only entries for enabled modules are registered.

## Controls

| Cache variable | Default | Effect |
| --- | :---: | --- |
| `LIBGS_PERFORMANCE_SCALE` | `1` | Multiplies measured work without changing the payload or topology |
| `LIBGS_PERFORMANCE_TIMEOUT` | `60` | CTest timeout per executable, in seconds |

Each report includes throughput, time per operation, elapsed time, operation
count, unit, and configured scale. Samples use warmups and repeated measurements
where the benchmark requires them; zero-operation samples and non-advancing
timers fail instead of emitting invalid rates.

## Target one measurement

Select one executable with a CTest name expression:

```sh
ctest --test-dir build-perf \
  -R '^libgs\.performance\.http-protocol$' -V
```

Performance executables use the common runner environment for exact case
selection when an executable contains multiple measurements:

```sh
LIBGS_TEST_CASE="HTTP loopback throughput" \
  build-perf/output/bin/libgs.test.performance.http
```

The soft-bus executable also provides a focused connection-management path:

```sh
build-perf/output/bin/libgs.test.performance.sbus --connection-only
```

## Compare results

- Compare runs from the same host, compiler, build type, feature set, and scale.
- Keep other machine load low and record CPU frequency/power policy when the
  distinction matters.
- Treat a single result as a measurement, not a regression threshold.
- Use repeated runs and a project-specific baseline before declaring a change.
- Do not combine Performance with ASan/UBSan or TSan; CMake omits this suite from
  sanitizer builds because instrumentation changes the measurements.
