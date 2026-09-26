# LibGS Tests

Tests are grouped by purpose. CTest entries carry the corresponding suite label.

| Suite | Purpose | Enable |
| --- | --- | --- |
| Functional | Deterministic public behavior, errors, ownership, cancellation, state | `BUILD_TESTING=ON` |
| Stress | Correctness under concurrency, saturation, and repeated lifecycle work | `LIBGS_BUILD_STRESS_TESTS=ON` |
| Fuzz | Input and call-sequence exploration with libFuzzer/ASan/UBSan | `LIBGS_BUILD_FUZZERS=ON` |
| Performance | Throughput and latency measurements without fixed thresholds | `LIBGS_BUILD_PERFORMANCE_TESTS=ON` |
| CMake | Module/option constraints and installed-package consumption | `LIBGS_BUILD_CMAKE_TESTS=ON` |

Only enabled library modules contribute tests.

## CMake integration tests

`LIBGS_BUILD_CMAKE_TESTS` follows `BUILD_TESTING` by default.  The configure
suite enumerates every Core/Coroutine/HTTP/WebSocket/Utilities module
combination, checks that invalid dependency combinations are rejected for the
documented reason, and verifies the generated package component state.  It also
checks the constraints between sanitizer, fuzz, stress, performance, provider,
and numeric test options.  Package probes verify that component selection
restores only its transitive external dependencies and reports unavailable
components before attempting unrelated dependency discovery.

The install-consumer test installs the current build into an isolated prefix.
An independent downstream project then uses `find_package(LibGS COMPONENTS
...)`, builds against every installed `LibGS::` target and the legacy `gs.`
targets, and runs the resulting executables.  Run these tests after building:

```sh
cmake -S . -B build-cmake-test -DBUILD_TESTING=ON
cmake --build build-cmake-test --parallel
ctest --test-dir build-cmake-test -L cmake --output-on-failure
```

## Functional tests

```sh
cmake -S . -B build-test -DBUILD_TESTING=ON \
  -DLIBGS_BUILD_HTTP=ON \
  -DLIBGS_BUILD_WEBSOCKET=ON \
  -DLIBGS_BUILD_UTILITIES=ON \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build-test --parallel
ctest --test-dir build-test -L functional --output-on-failure
```

CTest names are `libgs.<area>`; executables are
`build-test/output/bin/libgs.test.<area>`. See the
[functional API coverage map](functional/API_COVERAGE.md) for the source and
behavior assigned to each executable.

When Python 3.8+ is available and the build is not cross-compiling, CMake may
register local interoperability tests:

- HTTP uses local `curl`, falling back to Python's standard library.
- WebSocket uses the first available backend among Node.js `ws`, Python
  `websockets`, Python `websocket-client`, or `wscat`.

No dependency is downloaded. Select these tests with `-L interop`.

## Stress tests

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

CTest names are `libgs.stress.<module>`. The suite covers Core queues, locks,
the fallback joining-thread/stop-state implementation, coroutine
synchronization, repeated HTTP/WebSocket connections, utility lifecycle/fanout,
and optional UDP soft-bus pressure. Entries run serially at the CTest level;
concurrency occurs inside each executable.

## Fuzz tests

Fuzzing requires Clang with libFuzzer and a dedicated build. Functional, Stress,
and Performance sources are not added to this configuration.

```sh
cmake -S . -B build-fuzz -DBUILD_TESTING=ON \
  -DLIBGS_BUILD_FUZZERS=ON \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DLIBGS_BUILD_HTTP=ON \
  -DLIBGS_BUILD_WEBSOCKET=ON \
  -DLIBGS_BUILD_UTILITIES=ON \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-fuzz --parallel
ctest --test-dir build-fuzz -L fuzz --output-on-failure
```

Targets and CTest entries are `libgs.fuzz.<module>.<harness>`; binaries are in
`build-fuzz/output/fuzz/`. Seed corpora and dictionaries live under
`test/fuzz/`. CTest copies corpora into the build tree and retains crash
artifacts under `build-fuzz/test/fuzz/artifacts/`.

Run a longer campaign directly:

```sh
build-fuzz/output/fuzz/libgs.fuzz.core.public-api \
  -max_total_time=300 \
  -artifact_prefix=build-fuzz/test/fuzz/artifacts/core-public-api/ \
  build-fuzz/test/fuzz/corpus/core-public-api/
```

## Performance tests

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

CTest names are `libgs.performance.<area>`. Measurements cover Core
algorithms/queues/locks, coroutine primitives, HTTP, WebSocket, logging,
signal/slot, and soft bus. Compare runs only with the same host, compiler, build
type, feature set, and scale.

## Sanitizers

Functional and optional Stress tests can instrument the enabled LibGS modules:

| Switch | Instrumentation | Compilers |
| --- | --- | --- |
| `LIBGS_ENABLE_TEST_SANITIZERS=ON` | ASan + UBSan | GCC or Clang with GNU-style driver |
| `LIBGS_ENABLE_TEST_TSAN=ON` | TSan | GCC or Clang with GNU-style driver |

```sh
cmake -S . -B build-asan -DBUILD_TESTING=ON \
  -DLIBGS_ENABLE_TEST_SANITIZERS=ON \
  -DLIBGS_BUILD_STRESS_TESTS=ON \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-asan --parallel
ctest --test-dir build-asan -L sanitizer --output-on-failure
```

Use `LIBGS_ENABLE_TEST_TSAN=ON` in a separate build for TSan. The two sanitizer
switches are mutually exclusive, require `BUILD_TESTING=ON`, and cannot be
combined with LTO, Fuzz, or performance tests. CMake rejects these incompatible
configurations instead of silently omitting requested instrumentation or tests.
On Linux, CMake uses `setarch -R` per test when available to avoid incompatible
TSan shadow-memory layouts.

## Selection and controls

List or select CTest entries:

```sh
ctest --test-dir build-test -N
ctest --test-dir build-test -L functional --output-on-failure
ctest --test-dir build-test -R '^libgs\.http\.protocol$' --output-on-failure
```

Functional and Performance executables read runner environment variables.
Stress executables also expose equivalent command-line options:

| Environment | Option | Meaning |
| --- | --- | --- |
| `LIBGS_TEST_CASE` | `--case <name>` | Run an exact named case |
| `LIBGS_TEST_REPEAT` | `--repeat <count>` | Recreate and rerun the fixture |
| `LIBGS_TEST_SEED` | `--seed <value>` | Reproduce scheduling perturbations |
| `LIBGS_TEST_FAIL_FAST=1` | `--fail-fast` | Stop after the first failure |
| — | `--list` | List case names |

Suite cache controls:

| Suite | Variables (defaults) |
| --- | --- |
| Functional | `LIBGS_FUNCTIONAL_REPEAT=3`, `LIBGS_FUNCTIONAL_SEED=1`, `LIBGS_FUNCTIONAL_TIMEOUT=120` |
| Stress | `LIBGS_STRESS_SCALE=5`, `LIBGS_STRESS_REPEAT=3`, `LIBGS_STRESS_SEED=1`, `LIBGS_STRESS_TIMEOUT=180` |
| Fuzz | `LIBGS_FUZZ_SMOKE_RUNS=2048`, `LIBGS_FUZZ_SEED=1`, `LIBGS_FUZZ_MAX_LENGTH=4096`, `LIBGS_FUZZ_TIMEOUT=5`, `LIBGS_FUZZ_RSS_LIMIT_MB=1024` |
| Performance | `LIBGS_PERFORMANCE_SCALE=1`, `LIBGS_PERFORMANCE_TIMEOUT=60` |

Put deterministic contracts in Functional, pressure-dependent correctness in
Stress, broad input/state exploration in Fuzz, and measurements in Performance.
