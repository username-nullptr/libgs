# LibGS tests

The test suite has two layers:

- `functional`: correctness tests for each enabled module.
- `performance`: coarse local measurements for core transforms, lock-free queues,
  coroutine synchronization primitives, HTTP and WebSocket protocol/loopback
  work, and utility dispatch.

Coverage is organized by observable behavior rather than one test per function.
A representative public workflow is allowed to cover its small helpers; a separate
test is added only for a distinct boundary, error, state-transition, ownership, or
configuration branch. Performance tests likewise sample important dimensions
(for example protocol versus socket work, small versus large bodies, and reused
versus reconnected HTTP sessions) instead of building an unbounded cross-product.

The initially implemented WebSocket surface is covered as a bounded baseline.
Functional tests group opening-handshake validation, owned client/server API,
request snapshots and subprotocol negotiation, accept-queue behavior, and stream
state transitions. When OpenSSL support is enabled, a hermetic WSS loopback also
covers certificate verification and the TLS upgrade path. Frame/control/close
details that are already reached through a broader state test are not repeated as
standalone combinations.

Performance tests require the `libgs.functional` CTest fixture. Selecting only
the `performance` label therefore runs the functional suite first, and skips
the measurements if correctness does not pass.

For representative local measurements, use a Release build and verbose CTest
output:

```sh
cmake -S . -B build -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build -L functional --output-on-failure
ctest --test-dir build -L performance -V
```

The reported throughput and latency have no fixed pass threshold. They are
intended for rough comparisons on the same machine; a performance test fails
only when its result is functionally incorrect or it times out.

Core measurements cover percent encoding, URL parse/serialization, and SHA-1.
HTTP measurements separate parser/generator cost from loopback requests and,
when `LIBGS_HTTP_ZLIB_SUPPORT=ON`, include a gzip round trip. Loopback reporting
uses median-of-three samples for keep-alive small bodies, keep-alive 64 KiB
bodies, and reconnecting small requests.

WebSocket measurements keep the protocol and transport costs distinguishable:
they sample 4 KiB masking and frame parsing, then reuse one loopback connection
for 64 B and 64 KiB binary messages. They intentionally do not form a matrix of
roles, opcodes, fragmentation sizes, and connection lifetimes.

The coroutine synchronization measurements include raw atomic and immediate
awaitable baselines, uncontended acquire/release cycles, and queued waiter
wake-ups. This keeps runtime and primitive overhead distinguishable when
comparing two builds.

For memory errors and undefined behavior, use a separate ASan/UBSan build. The
instrumented build omits performance tests and should not be used for install
artifacts:

```sh
cmake -S . -B build-sanitize -DBUILD_TESTING=ON \
  -DLIBGS_ENABLE_TEST_SANITIZERS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-sanitize -j
ctest --test-dir build-sanitize -L sanitizer --output-on-failure
```

ASan/UBSan do not detect data races. Concurrency can be checked in a separate,
mutually exclusive TSan build:

```sh
cmake -S . -B build-tsan -DBUILD_TESTING=ON \
  -DLIBGS_ENABLE_TEST_TSAN=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-tsan -j
ctest --test-dir build-tsan -L sanitizer --output-on-failure
```

For repeated stability runs, append `--repeat until-fail:20` to either CTest
command.
