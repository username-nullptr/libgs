# Functional API coverage map

[Functional test guide](README.md) · [Test index](../README.md)

Functional tests own the public API contract. This map identifies where each
module's callable surface is exercised; it is intentionally organized by public
behavior rather than by implementation source file.

## Core

- `core/basic.cpp`: algorithms, hashes, UUIDs, byte order, string tools,
  optional/expected values, buffer helpers, spin/shared locks, and type utilities.
- `core/facilities.cpp`: value/string containers, URL parse/resolve, command-line
  parsing, INI memory/file/token APIs, lock-free queue variants, MIME detection,
  and application paths/environment.
- `core/runtime.cpp`: global/local execution, dispatch/post/delay, timers,
  cancellation, event pumps, futures, awaitables, public `post_completion`,
  associated immediate executors, and exception recovery.
- `core/public_api.cpp`: flags, parameter containers, value conversion overloads,
  formatting, endpoint wrappers, environment operations, dynamic libraries, and
  compile-time public type properties.

## Coroutine

- `coro/basic.cpp`: mutex/unique-lock, semaphore, condition variable,
  shared-mutex, timed operations, executor selection, sleep, and future/thread
  waiting APIs, including contention and timeout behavior.

## HTTP

- `http/basic.cpp`: protocol enums/cookies, client URL/proxy configuration,
  connection read/write accounting, file bodies, connection pools, and malformed
  reply recovery.
- `http/protocol.cpp`: incremental request/response parsers, generators, ranges,
  multipart bodies, conditions, upgrades, form data, authentication, and cookie
  storage policy.
- `http/public_api.cpp`: endpoint/value containers, client config, every named
  client request/context convenience method, compression policy/codecs, file
  option tokens, and sessions.
- `http/loopback.cpp`: owned server/client request and response APIs, route
  updates, keep-alive, and static-file cache behavior.
- `interop/http_interop.py`: independent client/server compatibility when a local
  backend is available.

## WebSocket

- `websocket/protocol.cpp`: errors, randomness, opcode/close helpers, masking,
  frame generation/parsing, fragmentation, and opening handshakes.
- `websocket/public_api.cpp`: umbrella types, configuration/value objects,
  executor-bound objects, completion signatures, callbacks, and retry result APIs.
- `websocket/handshake.cpp`: HTTP upgrade, proxies, redirects, validators,
  extension/subprotocol negotiation, and timeout bounds.
- `websocket/client_server.cpp`: owned client/server lifecycle, accept queues,
  cancellation, retry recovery, simultaneous close, and compression negotiation.
- `websocket/stream.cpp`: sync/async reads and writes, frame/message boundaries,
  control callbacks, queue limits, cancellation, automatic ping/pong, compression,
  close deadlines, and lifecycle transitions.
- `websocket/wss.cpp`: TLS verification and secure upgrade when OpenSSL is enabled.
- `interop/websocket_interop.py`: independent implementation compatibility when a
  local backend is available.

## Utilities

- `utils/modules.cpp`: umbrella/module compile and initialization surface.
- `utils/basic.cpp`: signal-slot modes, observer ownership, mutation, blocking,
  argument ownership, synchronous/asynchronous/awaitable delivery.
- `utils/runtime.cpp`: settings persistence and async tokens, process lifecycle,
  pipes/environment/cancellation, local message bus, and logger configuration/I/O.
- `utils/sbus.cpp`: raw/typed publish-subscribe, cancellation, topic indexing,
  large-payload fanout ownership, UDP scope/fragmentation, and receive statistics.

## Review checklist

When a public declaration is added or changed:

1. Add a successful call through the public header.
2. Cover each meaningfully different overload/token family; use `static_assert`
   for compile-time-only constraints.
3. Add invalid-input/error assertions for public validation behavior.
4. Cover ownership, cancellation, timeout, and state-transition branches when
   the API exposes them.
5. Add a stress case only if correctness depends on pressure or concurrency.
6. Add a fuzz operation when input or call ordering can explore more states than
   deterministic examples.
7. Add a performance sample only for an explicitly performance-sensitive path.
