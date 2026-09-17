# LibGS tests

The test tree is split by purpose. A test belongs to exactly one primary suite;
do not use iteration count alone to decide where it belongs.

| Suite | Purpose | Default build |
| --- | --- | --- |
| `functional` | Deterministic correctness of every supported public API call, overload family, error path, and state transition. | Yes |
| `stress` | High-pressure stability: concurrency, queue saturation, repeated connections, fanout, and long call sequences. | Opt-in |
| `fuzz` | Sanitized, input-driven calls including malformed values and unusual API call sequences; not limited to protocol parsers. | Dedicated opt-in build |
| `performance` | Repeatable measurements for complexity-sensitive algorithms, synchronization overhead, and I/O throughput/latency. | Opt-in |

All CTest entries carry the primary suite name as a label. `ctest -L <suite>`
therefore selects one category without implicitly running another category.
Functional target and CTest names remain compatible with the historical names.

## Functional

Functional tests are the API contract. Every new or changed public callable must
be exercised here. Compile-time-only overloads and constraints may be covered by
`static_assert`; runtime behavior must be invoked and checked. A broad workflow
may cover small accessors, but boundaries, errors, ownership, cancellation, and
state transitions need explicit assertions.

Executor-bound public types also share a compile-time contract: both the standard
`executor_type` spelling and the compatibility `executor_t` spelling must exist
and denote the same type. This is checked across core, HTTP, WebSocket, and
utilities public objects.

The module-to-test map and review checklist are in
[`functional/API_COVERAGE.md`](functional/API_COVERAGE.md). HTTP and WebSocket
network behavior uses loopback endpoints. Optional interoperability tests use an
independent implementation already installed on the machine and never download a
dependency at configure time; these also carry the `interop` and `external`
labels.

```sh
cmake -S . -B build -DBUILD_TESTING=ON \
  -DLIBGS_BUILD_HTTP=ON -DLIBGS_BUILD_WEBSOCKET=ON \
  -DLIBGS_BUILD_UTILITIES=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build -L functional --output-on-failure
```

## Stress

Stress tests validate correctness while work is highly concurrent or repeatedly
recreated. They cover exact-once MPMC delivery in both circular and linked queues
under single-slot saturation, forced-eviction accounting during concurrent queue
access, concurrent URL/text work, coroutine mutex and shared-mutex reader/writer
storms, timed-waiter races, mixed HTTP keep-alive/reconnect traffic with payloads
through 64 KiB, repeated WebSocket handshakes with small text and 64 KiB binary
messages, concurrent signal mutation/emission, and message-bus fanout. They are
serial at the CTest level so two machine-saturating cases do not distort each
other.

HTTP and WebSocket stress cases run one strand per client and a strand for the
server-side connection state while several threads run the shared `io_context`.
This follows Asio's shared-object contract: concurrency is across independent
connections, while handlers touching one logical I/O object are serialized.

`LIBGS_STRESS_SCALE` is a positive integer work multiplier. The default is `4`;
larger values are intended for soak jobs. Repeating CTest is useful for detecting
rare scheduling failures.

```sh
cmake -S . -B build-stress -DBUILD_TESTING=ON \
  -DLIBGS_BUILD_STRESS_TESTS=ON -DLIBGS_STRESS_SCALE=4 \
  -DLIBGS_BUILD_HTTP=ON -DLIBGS_BUILD_WEBSOCKET=ON \
  -DLIBGS_BUILD_UTILITIES=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-stress -j
ctest --test-dir build-stress -L stress --repeat until-fail:20 \
  --output-on-failure
```

The local message bus has bounded per-subscriber queues. Its stress case sends
concurrent bursts within that documented capacity and waits between bursts;
intentional `force_enqueue` eviction is not reported as data corruption.

## Fuzz

Fuzzing has a dedicated Clang/libFuzzer build with ASan and UBSan. Harnesses cover
URL and value manipulation, coroutine synchronization call sequences, HTTP
request/response parsers and public value APIs, WebSocket frames/handshakes and
public value APIs, and utility signal connect/disconnect/block/emit sequences.
Expected API rejections are handled; sanitizer findings, invariant violations,
and unexpected failures still terminate the harness.

Smoke tests start from checked-in structured corpora and protocol dictionaries;
CMake copies corpora into the build tree so libFuzzer can minimize or extend them
without modifying the source tree. Harnesses use metamorphic checks where an
exact oracle is available, including fragmented-versus-contiguous HTTP parsing,
generated HTTP request/response round trips and parser reuse, URL serialization
stability, HTTP range round trips, generated WebSocket frame round trips,
WebSocket mask involution, sticky parser errors/reset, signal connection-state
modeling, and generated-handshake round trips.

Every fuzzer is also a bounded CTest smoke test. `LIBGS_FUZZ_SMOKE_RUNS` controls
its iteration count (default `2048`). Longer local or CI campaigns can invoke a
fuzzer binary directly with `-max_total_time` or `-runs` and a persistent corpus.

```sh
cmake -S . -B build-fuzz -DBUILD_TESTING=ON \
  -DLIBGS_BUILD_FUZZERS=ON -DCMAKE_CXX_COMPILER=clang++ \
  -DLIBGS_BUILD_HTTP=ON -DLIBGS_BUILD_WEBSOCKET=ON \
  -DLIBGS_BUILD_UTILITIES=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-fuzz -j
ctest --test-dir build-fuzz -L fuzz --output-on-failure

# Example longer campaign:
build-fuzz/output/fuzz/libgs.fuzz.core.public-api \
  -max_total_time=300 corpus/core-public-api
```

Fuzz harnesses stay within the C++ API's callable domain: they deliberately pass
malformed values and surprising sequences, but do not manufacture invalid
pointers or violate object lifetimes in the harness itself. That distinction lets
sanitizers attribute undefined behavior to the library rather than to test code.

## Performance

Performance tests are measurements, not soak tests. They use warmups and stable
sample shapes, validate their results, and report throughput/latency without a
machine-independent pass threshold. Run them in Release mode on a quiet machine
and compare results from the same host.

Coverage includes input-size samples for algorithmic scaling, lock-free queues,
coroutine synchronization, HTTP parser/generator and loopback I/O, WebSocket
codec and loopback I/O, logger formatting/dispatch/file output, signal-slot
dispatch, and local message-bus fanout.

```sh
cmake -S . -B build-perf -DBUILD_TESTING=ON \
  -DLIBGS_BUILD_PERFORMANCE_TESTS=ON \
  -DLIBGS_BUILD_HTTP=ON -DLIBGS_BUILD_WEBSOCKET=ON \
  -DLIBGS_BUILD_UTILITIES=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-perf -j
ctest --test-dir build-perf -L performance -V
```

## Sanitizers

ASan/UBSan and TSan builds are mutually exclusive. Performance tests are omitted
from sanitizer builds because instrumentation invalidates their measurements;
functional and explicitly enabled stress tests remain available.

```sh
cmake -S . -B build-sanitize -DBUILD_TESTING=ON \
  -DLIBGS_ENABLE_TEST_SANITIZERS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-sanitize -j
ctest --test-dir build-sanitize -L sanitizer --output-on-failure

cmake -S . -B build-tsan -DBUILD_TESTING=ON \
  -DLIBGS_ENABLE_TEST_TSAN=ON -DLIBGS_BUILD_STRESS_TESTS=ON \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-tsan -j
ctest --test-dir build-tsan -L sanitizer --output-on-failure
```
