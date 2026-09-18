# LibGS Tests

The test tree is organized by verification purpose. Each test has one primary
suite, and every CTest entry carries that suite name as a label.

## Suite map

| Suite | Use it for | Build mode | Guide |
| --- | --- | --- | --- |
| Functional | Deterministic public API behavior, errors, ownership, cancellation, and state transitions | Included when `BUILD_TESTING=ON` | [Functional tests](functional/README.md) |
| Stress | Correctness under concurrency, queue pressure, repeated connections, and repeated lifecycle work | Opt in with `LIBGS_BUILD_STRESS_TESTS=ON` | [Stress tests](stress/README.md) |
| Fuzz | Sanitized, input-driven API calls and state-machine sequences | Dedicated Clang/libFuzzer build | [Fuzz tests](fuzz/README.md) |
| Performance | Repeatable throughput and latency measurements | Opt in with `LIBGS_BUILD_PERFORMANCE_TESTS=ON` | [Performance tests](performance/README.md) |
| Sanitizers | Functional and optional Stress suites under ASan/UBSan or TSan | Dedicated instrumented build | [Sanitizer builds](SANITIZERS.md) |

## Quick functional run

This configuration enables every module so that all module-level functional
groups are registered:

```sh
cmake -S . -B build-test -DBUILD_TESTING=ON \
  -DLIBGS_BUILD_HTTP=ON \
  -DLIBGS_BUILD_WEBSOCKET=ON \
  -DLIBGS_BUILD_UTILITIES=ON \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build-test --parallel
ctest --test-dir build-test -L functional --output-on-failure
```

Only tests for enabled modules are built. Core is always present; Coroutine is
enabled by default; HTTP, WebSocket, Utilities, TLS, and compression follow the
same build switches as the library.

## Select tests

Use CTest labels to select a suite:

```sh
ctest --test-dir build-test -N
ctest --test-dir build-test -N -L functional
ctest --test-dir build-test -L functional --output-on-failure
```

Use a CTest name expression for one executable:

```sh
ctest --test-dir build-test \
  -R '^libgs\.websocket\.stream$' --output-on-failure
```

CTest names and executable names follow these patterns:

| Suite | CTest name | Executable |
| --- | --- | --- |
| Functional | `libgs.<name>` | `build/output/bin/libgs.test.<name>` |
| Stress | `libgs.stress.<module>` | `build/output/bin/libgs.test.stress.<module>` |
| Fuzz | `libgs.fuzz.<module>.<name>` | `build/output/fuzz/libgs.fuzz.<module>.<name>` |
| Performance | `libgs.performance.<name>` | `build/output/bin/libgs.test.performance.<name>` |

The executable paths shown here are for single-config generators. Multi-config
generators may add a configuration directory such as `Release/`.

## Common test runner

Functional, Stress, and Performance executables use `test/test.h`. It prints
each case duration and reports the case, iteration, and derived seed on failure.

| Environment variable | Command-line option | Meaning |
| --- | --- | --- |
| `LIBGS_TEST_CASE` | `--case <name>` | Run one exact case name; the command-line option may be repeated |
| `LIBGS_TEST_REPEAT` | `--repeat <count>` | Recreate the complete case fixture for each run |
| `LIBGS_TEST_SEED` | `--seed <value>` | Reproduce scheduling perturbations |
| `LIBGS_TEST_FAIL_FAST=1` | `--fail-fast` | Stop after the first failed run |
| — | `--list` | List cases without running them |
| — | `--help` | Show runner options |

Stress executables expose the command-line options. Functional and Performance
executables read the environment variables. CTest supplies suite-specific
repeat, seed, scale, and timeout values from the CMake cache.

Example direct functional invocation:

```sh
LIBGS_TEST_CASE="arithmetic and byte order" \
LIBGS_TEST_REPEAT=100 LIBGS_TEST_SEED=42 \
  build-test/output/bin/libgs.test.core
```

## Choosing a suite

- Put stable success, error, lifetime, and state-transition assertions in
  Functional.
- Put correctness that requires pressure, concurrency, or repeated lifecycle
  reconstruction in Stress.
- Put broad malformed-input or call-sequence exploration in Fuzz.
- Put measurements for explicitly performance-sensitive paths in Performance.
- Use a Sanitizer build to instrument Functional and Stress; it is not a
  separate source-test category.

The [functional coverage map](functional/API_COVERAGE.md) records where each
public API area is exercised.
