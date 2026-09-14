# LibGS tests

The test suite has three layers:

- `functional`: correctness tests for each enabled module.
- `fuzz`: input-driven Clang libFuzzer harnesses, built separately from the
  functional suite.
- `performance`: coarse local measurements for core transforms, lock-free queues,
  coroutine synchronization primitives, HTTP and WebSocket protocol/loopback
  work, and utility dispatch.

Coverage is organized by observable behavior rather than one test per function.
A representative public workflow is allowed to cover its small helpers; a separate
test is added only for a distinct boundary, error, state-transition, ownership, or
configuration branch. Performance tests likewise sample important dimensions
(for example protocol versus socket work, small versus large bodies, and reused
versus reconnected HTTP sessions) instead of building an unbounded cross-product.

The HTTP/1.1 WebSocket implementation is covered by protocol, public-API,
handshake, owned client/server, stream, and optional WSS groups. Coverage includes
incremental frame parsing across a deterministic chunk corpus, opening-handshake
validation, cross-origin redirect credential handling, request snapshots,
subprotocol and RFC 7692 `permessage-deflate` negotiation, asynchronous
Upgrade validators, request-aware synchronous/asynchronous selectors,
frame-level reads, accept-queue capacity/FIFO/timeout
behavior, real handshake deadlines, simultaneous Close, and stream state
transitions. When OpenSSL support is enabled, a hermetic WSS loopback covers
certificate verification and the TLS upgrade path. When WebSocket zlib support
is enabled, a fragmented compressed loopback and invalid compressed payload are
also covered. Frame/control/close details already reached through a broader state
test are not repeated as standalone combinations.

HTTP and WebSocket network tests prefer independent implementations already
available on the machine, without downloading test dependencies. HTTP uses
`curl` when present (or Python's standard HTTP stack) against the LibGS server,
then checks the LibGS client against a Python HTTP server. WebSocket prefers the
Node `ws` package, which exercises both client/server directions; Python
`websockets`, `websocket-client`, and `wscat` are accepted client-side fallbacks.
CMake reports the selected backend. If Python or a WebSocket implementation is
not available, the dependency-free LibGS loopback tests remain the fallback.
Interoperability tests carry the `interop` and `external` CTest labels.

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

The utility logger measurements separate formatting from dispatch. They cover
cached and named disabled calls, four-thread named calls, an enabled null sink,
and asynchronous daily-file enqueue throughput followed by an explicit flush.
The null sink deliberately removes terminal I/O while preserving the enabled
logger path; the file measurement uses a temporary directory and validates that
data reached the sink.

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

Input-oriented coverage is kept in Clang libFuzzer harnesses rather than the
functional suite. Core covers URL parsing and percent-encoding round trips;
HTTP covers incremental request and response parsing; WebSocket covers
incremental frames and opening handshakes. Build the harnesses separately from
the functional suite and other sanitizer modes, then run them locally with a
bounded iteration count or duration:

```sh
cmake -S . -B build-fuzz -DBUILD_TESTING=ON \
  -DLIBGS_BUILD_FUZZERS=ON -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-fuzz --target \
  libgs.fuzz.core.url \
  libgs.fuzz.http.request-parser \
  libgs.fuzz.http.response-parser \
  libgs.fuzz.websocket.frame-parser \
  libgs.fuzz.websocket.handshake-parser -j
build-fuzz/output/fuzz/libgs.fuzz.core.url -runs=10000
build-fuzz/output/fuzz/libgs.fuzz.http.request-parser -runs=10000
build-fuzz/output/fuzz/libgs.fuzz.http.response-parser -runs=10000
build-fuzz/output/fuzz/libgs.fuzz.websocket.frame-parser -runs=10000
build-fuzz/output/fuzz/libgs.fuzz.websocket.handshake-parser -runs=10000
```
