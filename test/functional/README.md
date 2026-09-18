# Functional Tests

[Test index](../README.md) · [Public API coverage map](API_COVERAGE.md)

Functional tests define the deterministic public API contract. They cover
successful calls, validation failures, ownership, cancellation, timeouts, and
state transitions without requiring sustained machine pressure.

## Build and run

```sh
cmake -S . -B build-test -DBUILD_TESTING=ON \
  -DLIBGS_BUILD_HTTP=ON \
  -DLIBGS_BUILD_WEBSOCKET=ON \
  -DLIBGS_BUILD_UTILITIES=ON \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build-test --parallel
ctest --test-dir build-test -L functional --output-on-failure
```

Tests are registered only for enabled modules. Add
`LIBGS_OPENSSL_SUPPORT=ON` to include WSS coverage and the corresponding
OpenSSL dependency.

## Test groups

| Module | CTest entries | Scope |
| --- | --- | --- |
| Core | `libgs.core`, `libgs.core.facilities`, `libgs.core.runtime`, `libgs.core.public-api` | Algorithms, containers, runtime, configuration, system facilities, and public overloads |
| Coroutine | `libgs.coro` | Awaitable synchronization, timeout, executor, and future/thread waiting behavior |
| HTTP | `libgs.http`, `libgs.http.protocol`, `libgs.http.public-api`, `libgs.http.loopback` | Protocol, client, server, file, session, connection, and loopback behavior |
| WebSocket | `libgs.websocket.protocol`, `libgs.websocket.public-api`, `libgs.websocket.handshake`, `libgs.websocket.client-server`, `libgs.websocket.stream`, optional `libgs.websocket.wss` | RFC 6455 codec, Upgrade, owned APIs, stream lifecycle, and TLS |
| Utilities | `libgs.utils`, `libgs.utils.runtime`, `libgs.utils.sbus`, `libgs.utils.modules` | Signals, observers, settings, processes, logger, modules, and soft bus |
| Interoperability | `libgs.http.interop`, `libgs.websocket.interop` when a backend is available | Compatibility with an independent local implementation |

All names in this table are complete CTest names. The exact
source-to-capability mapping is maintained in
[API_COVERAGE.md](API_COVERAGE.md).

## CMake controls

| Cache variable | Default | Effect |
| --- | :---: | --- |
| `LIBGS_FUNCTIONAL_REPEAT` | `1` | Runs every selected case this many times |
| `LIBGS_FUNCTIONAL_SEED` | `1` | Base seed used to derive a reproducible per-case seed |
| `LIBGS_FUNCTIONAL_TIMEOUT` | `60` | Default CTest timeout per executable, in seconds |

CTest sets fail-fast mode for each functional executable. Selected network tests
use shorter 15-second timeouts in normal builds and the suite timeout under
sanitizers. Interoperability tests use a 30-second timeout.

## Run one executable or case

Select one CTest entry by its complete name:

```sh
ctest --test-dir build-test \
  -R '^libgs\.http\.protocol$' --output-on-failure
```

Functional executables accept case selection through environment variables:

```sh
LIBGS_TEST_CASE="arithmetic and byte order" \
LIBGS_TEST_REPEAT=100 LIBGS_TEST_SEED=42 \
  build-test/output/bin/libgs.test.core
```

Case names are the strings registered at the bottom of each test source. A
non-matching name exits with status 2.

## Interoperability tests

Loopback tests are always the network baseline and do not download dependencies.
When Python 3.8+ is available and the build is not cross-compiling, CMake may
register independent-client checks:

- HTTP uses local `curl` when found, otherwise Python's standard library.
- WebSocket selects the first available backend from Node.js `ws`, Python
  `websockets`, Python `websocket-client`, or `wscat`.

Interoperability entries carry the `functional`, `interop`, and `external`
labels:

```sh
ctest --test-dir build-test -L interop --output-on-failure
```

If no external WebSocket implementation is installed, the WebSocket interop
entry is not registered.

## Adding or changing coverage

For every new or changed public callable:

1. Call it through a public header.
2. Cover meaningfully different overload and completion-token families.
3. Assert invalid input and error reporting.
4. Cover ownership, cancellation, timeout, and state changes where exposed.
5. Use `static_assert` for compile-time-only contracts.
6. Update [API_COVERAGE.md](API_COVERAGE.md).

Move a scenario to another suite only when its primary purpose changes: pressure
belongs in [Stress](../stress/README.md), broad input exploration in
[Fuzz](../fuzz/README.md), and measurements in
[Performance](../performance/README.md).
