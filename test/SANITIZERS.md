# Sanitizer Builds

[Test index](README.md) · [Stress tests](stress/README.md) ·
[Fuzz tests](fuzz/README.md)

Sanitizer options instrument the enabled LibGS modules and the Functional and
optional Stress test executables. ASan/UBSan and TSan use separate build trees.

## Valid configurations

| Option | Instrumentation | Compiler support |
| --- | --- | --- |
| `LIBGS_ENABLE_TEST_SANITIZERS=ON` | AddressSanitizer + UndefinedBehaviorSanitizer | GCC or Clang with a GNU-style driver |
| `LIBGS_ENABLE_TEST_TSAN=ON` | ThreadSanitizer | GCC or Clang with a GNU-style driver |

The following rules are enforced by CMake:

- `BUILD_TESTING` must be ON.
- The two sanitizer options cannot be enabled together.
- MSVC and non-GNU-style compiler drivers are not accepted.
- `LIBGS_ENABLE_LTO` must be OFF.
- Fuzzers use their own dedicated instrumentation and cannot be combined with
  either test sanitizer option.
- Performance tests are omitted because instrumentation invalidates their
  measurements.

## ASan and UBSan

```sh
cmake -S . -B build-asan -DBUILD_TESTING=ON \
  -DLIBGS_ENABLE_TEST_SANITIZERS=ON \
  -DLIBGS_BUILD_HTTP=ON \
  -DLIBGS_BUILD_WEBSOCKET=ON \
  -DLIBGS_BUILD_UTILITIES=ON \
  -DLIBGS_BUILD_STRESS_TESTS=ON \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-asan --parallel
ctest --test-dir build-asan -L sanitizer --output-on-failure
```

## TSan

```sh
cmake -S . -B build-tsan -DBUILD_TESTING=ON \
  -DLIBGS_ENABLE_TEST_TSAN=ON \
  -DLIBGS_BUILD_HTTP=ON \
  -DLIBGS_BUILD_WEBSOCKET=ON \
  -DLIBGS_BUILD_UTILITIES=ON \
  -DLIBGS_BUILD_STRESS_TESTS=ON \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-tsan --parallel
ctest --test-dir build-tsan -L sanitizer --output-on-failure
```

Use `-L functional` or `-L stress` in an instrumented build to isolate one
primary suite. Every instrumented entry also carries the `sanitizer` label and
runs serially.

## Runtime settings

CTest applies these environments:

| Build | Environment |
| --- | --- |
| ASan/UBSan | `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1:abort_on_error=1` |
| ASan/UBSan | `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1` |
| TSan | `TSAN_OPTIONS=halt_on_error=1` |

CTest also fails a test when its output contains an AddressSanitizer,
LeakSanitizer, ThreadSanitizer, or UBSan runtime-error marker.

## Linux TSan launcher

On Linux, configuration probes `setarch`. When the host permits
`setarch <architecture> -R`, CTest disables ASLR only for each test process.
The setting is inherited by interoperability peers launched from that process,
while the host and non-TSan builds remain unchanged.

If the probe is unavailable or rejected, tests run without the launcher. A GCC
TSan shadow-memory mapping failure in that configuration indicates a host/kernel
layout conflict rather than a LibGS test assertion; use a host that permits the
per-process launcher or a compatible Clang TSan environment.

## Fuzzer instrumentation

Fuzz targets always use libFuzzer + ASan + UBSan in their own build and carry
both `fuzz` and `sanitizer` labels. Configure them with
`LIBGS_BUILD_FUZZERS=ON` instead of either option on this page; see the
[Fuzz guide](fuzz/README.md).
