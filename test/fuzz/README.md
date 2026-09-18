# Fuzz Tests

[Test index](../README.md) · [Sanitizer builds](../SANITIZERS.md)

Fuzzing uses Clang's libFuzzer with ASan and UBSan. Harnesses pass malformed
values and unusual operation sequences through callable public APIs without
manufacturing invalid pointers or violating object lifetimes in the harness.

## Dedicated build

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

A fuzzer build has these constraints:

- `BUILD_TESTING` must be ON.
- The C++ compiler must be Clang with libFuzzer.
- `LIBGS_ENABLE_TEST_SANITIZERS` and `LIBGS_ENABLE_TEST_TSAN` must be OFF.
- Stress and Performance suites must be OFF.
- Functional, Stress, and Performance sources are not added to this build.

LibGS libraries receive `fuzzer-no-link,address,undefined` instrumentation;
each harness links `fuzzer,address,undefined`.

## Harness map

| Module | Harnesses | Input/state explored |
| --- | --- | --- |
| Core | `url`, `public-api`, `queue` | URL parsing/serialization, values and public operations, queue sequences checked against a deque model |
| Coroutine | `sync-primitives` | Mutex, semaphore, and synchronization call sequences |
| HTTP | `request-parser`, `response-parser`, `roundtrip`, `public-api` | Incremental parsers, generated round trips, ranges, and public value operations |
| WebSocket | `frame-parser`, `handshake-parser`, `public-api` | Frames, handshakes, masking, parser reset, close values, and extensions |
| Utilities | `signal-sequence` | Connect, disconnect, block, unblock, and emit sequences |

Targets and CTest entries use
`libgs.fuzz.<module>.<harness>`. Binaries are written under
`build-fuzz/output/fuzz/`.

## Smoke-test controls

Every harness is registered as a bounded CTest smoke test.

| Cache variable | Default | Effect |
| --- | :---: | --- |
| `LIBGS_FUZZ_SMOKE_RUNS` | `2048` | libFuzzer iterations per CTest entry |
| `LIBGS_FUZZ_SEED` | `1` | Base libFuzzer seed |
| `LIBGS_FUZZ_MAX_LENGTH` | `4096` | Maximum generated input size in bytes |
| `LIBGS_FUZZ_TIMEOUT` | `5` | Per-input libFuzzer timeout in seconds |
| `LIBGS_FUZZ_RSS_LIMIT_MB` | `1024` | Memory limit in MiB |

CTest gives each fuzzer a 60-second outer timeout, runs fuzz entries serially,
and labels them `fuzz` and `sanitizer`.

## Corpora, dictionaries, and artifacts

Seed corpora live in `test/fuzz/corpus/`; dictionaries live in
`test/fuzz/dictionaries/`. CMake copies each selected corpus into:

```text
build-fuzz/test/fuzz/corpus/<module>-<harness>/
```

CTest and longer local campaigns mutate the build-tree copy, leaving checked-in
seeds unchanged. Crash and timeout artifacts from CTest are retained under:

```text
build-fuzz/test/fuzz/artifacts/<module>-<harness>/
```

## Longer campaign

Invoke a harness directly with a persistent build-tree corpus:

```sh
build-fuzz/output/fuzz/libgs.fuzz.core.public-api \
  -max_total_time=300 \
  -artifact_prefix=build-fuzz/test/fuzz/artifacts/core-public-api/ \
  build-fuzz/test/fuzz/corpus/core-public-api/
```

Use `-runs=<count>` instead of `-max_total_time` for an iteration-bounded
campaign. Keep a reproducer in the matching source corpus only after it is
reduced and suitable for version control.

## Harness rules

- Treat expected parser and API rejection as normal results.
- Terminate on sanitizer findings, invariant failures, and unexpected errors.
- Prefer round-trip, metamorphic, or reference-model checks over crash-only
  fuzzing when an oracle is available.
- Keep input length bounded by `LIBGS_FUZZ_MAX_LENGTH`.
- Add structured seeds and protocol tokens to the matching corpus and
  dictionary.
