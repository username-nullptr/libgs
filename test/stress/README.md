# Stress Tests

[Test index](../README.md) · [Sanitizer builds](../SANITIZERS.md)

Stress tests verify correctness when work is concurrent, queues are saturated,
connections are repeated, or complete object lifecycles are recreated many
times. They are correctness tests, not performance benchmarks.

## Build and run

```sh
cmake -S . -B build-stress -DBUILD_TESTING=ON \
  -DLIBGS_BUILD_STRESS_TESTS=ON \
  -DLIBGS_BUILD_HTTP=ON \
  -DLIBGS_BUILD_WEBSOCKET=ON \
  -DLIBGS_BUILD_UTILITIES=ON \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-stress --parallel
ctest --test-dir build-stress -L stress --output-on-failure
```

One executable is built for each enabled module. Stress entries are marked
`RUN_SERIAL` in CTest so separate machine-saturating programs do not distort
one another.

## Coverage

| Executable | Primary pressure |
| --- | --- |
| `libgs.test.stress.core` | Exact-once MPMC delivery, circular queue saturation/growth/resize, linked queue reclamation, forced eviction accounting, URL and text work |
| `libgs.test.stress.coro` | Mutex and shared-mutex reader/writer contention, semaphore and timed-waiter races |
| `libgs.test.stress.http` | Repeated client lifecycle and concurrent keep-alive/reconnect traffic with payloads through 64 KiB |
| `libgs.test.stress.websocket` | Repeated handshakes and concurrent text/binary echo connections |
| `libgs.test.stress.utils` | Signal mutation/emission, object lifecycle, and soft-bus fanout |

HTTP and WebSocket cases use one strand per client and a strand for server-side
connection state while multiple threads run the shared `io_context`. Concurrency
is across independent connections; access to one logical I/O object remains
serialized.

## Load controls

| Cache variable | Default | Effect |
| --- | :---: | --- |
| `LIBGS_STRESS_SCALE` | `4` | Multiplies work performed inside one fixture |
| `LIBGS_STRESS_REPEAT` | `1` | Recreates and reruns every case fixture |
| `LIBGS_STRESS_SEED` | `1` | Base seed for reproducible scheduling perturbations |
| `LIBGS_STRESS_TIMEOUT` | `180` | CTest timeout per stress executable, in seconds |

`SCALE` changes the compiled workload. `REPEAT` exercises construction,
shutdown, cancellation, and reclamation by rebuilding the complete fixture.
Keep them independent when isolating a failure.

Example lifecycle-focused configuration:

```sh
cmake -S . -B build-stress -DBUILD_TESTING=ON \
  -DLIBGS_BUILD_STRESS_TESTS=ON \
  -DLIBGS_STRESS_SCALE=1 \
  -DLIBGS_STRESS_REPEAT=100 \
  -DLIBGS_STRESS_SEED=42 \
  -DLIBGS_STRESS_TIMEOUT=600 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

## Target one case

Stress binaries expose the common runner command line:

```sh
build-stress/output/bin/libgs.test.stress.core --list

build-stress/output/bin/libgs.test.stress.core \
  --case "low-load queue lifecycle repetition" \
  --repeat 10000 --seed 42 --fail-fast
```

The failure output includes the exact case, repeat index, and derived seed. Use
the same base seed and repeat count to reproduce the same perturbation sequence.

## Queue and bus semantics

The circular and linked queue cases check exact-once delivery, order, capacity,
and reclamation invariants. Forced circular-queue eviction is checked through
its accounting contract rather than treated as corruption.

The local soft bus uses bounded per-subscriber queues. Its stress case sends
bursts within that capacity and waits between bursts; intentional
`force_enqueue` eviction is not a delivery-corruption signal.

## With sanitizers

Stress can be combined with ASan/UBSan or TSan. Sanitizer builds use the same
stress label, timeout, scale, repeat, and seed controls, and all instrumented
tests also receive the `sanitizer` label. See
[Sanitizer builds](../SANITIZERS.md) for valid option combinations.
