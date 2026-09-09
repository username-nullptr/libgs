# LibGS tests

The test suite has two layers:

- `functional`: correctness tests for each enabled module.
- `performance`: coarse local measurements for the lock-free queues, coroutine
  synchronization primitives, HTTP loopback requests, and utility dispatch.

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
