# Project Roadmap

Language: English | [Simplified Chinese](../zh_CN/roadmap.md)

LibGS is intended to be a reusable asynchronous application foundation, not a
library tied to a single network protocol. This document separates the current
implementation from planned direction.

Roadmap entries describe intent, not a release date or compatibility promise.

## Current implementation

| Area | Status | Scope |
| --- | :---: | --- |
| Core runtime | Implemented | Event loop, scheduling, timers, completion tokens, and executor-aware APIs |
| Core facilities | Implemented | Values, INI files, argument parsing, algorithms, containers, MIME checks, and system helpers |
| Coroutine support | Implemented | Awaitable waits, synchronization primitives, and executor switching |
| HTTP/HTTPS | Implemented | HTTP/1.0 and HTTP/1.1 clients, servers, protocol utilities, TLS, and optional gzip |
| Application utilities | Implemented | Logging, settings, signals, observers, modules, processes, and an extensible soft bus with a built-in in-process transport |
| WebSocket | Implemented | HTTP/1.1 WS/WSS, opening handshakes, message/frame/control I/O, fragmentation, optional constrained `permessage-deflate`, close, cancellation, timeouts, queues, and backpressure |

## WebSocket scope and planned extensions

The `gs.websocket` module implements the RFC 6455 HTTP/1.1 baseline and
integrates with LibGS execution, connection, TLS, and completion-token
conventions.

The implemented baseline includes:

- client and server roles;
- `ws://` and `wss://` transports;
- HTTP Upgrade validation and connection handover;
- text, binary, continuation, ping, pong, and close frames;
- fragmented-message assembly;
- data-frame reads alongside complete-message reads;
- asynchronous request and Origin validation during Upgrade;
- optional RFC 7692 `permessage-deflate` with context takeover disabled;
- callback and coroutine completion styles;
- cancellation, timeout, queue, and backpressure behavior; and
- clear ownership of buffers and handed-over connections.

`read()` returns complete data messages and `read_frame()` returns individual
data frames when no extension is negotiated; Ping/Pong events are retained while
reading continues; after a clean Close, subsequent reads return EOF. Concurrent operations must
follow the stream's documented serialization rules. The protocol parser is
incremental and borrows the input buffer; payload copying is limited to message
aggregation and transforms where ownership is required.

Planned extensions, outside the implemented scope, are:

- a general extension capability registry and additional RFC 7692 parameter
  profiles, including context takeover and window-bit negotiation;
- HTTP/2 and HTTP/3 extended CONNECT transports;
- optional WebSocket-level proxy configuration and authentication; and
- application-level keepalive, reconnect, and message-routing helpers.

## Existing preparation for upgrades

The HTTP server already exposes a protocol-neutral ownership boundary through
`service_context::hand_over_connection()`. It allows an accepted HTTP Upgrade
connection and pending input to leave the normal HTTP request lifecycle. This
keeps the HTTP parser independent of upgraded protocols. Applications should
normally use `websocket::upgrade()`, which performs the RFC 6455 handshake and
adopts the handed-over connection into a WebSocket stream.

## Architectural direction

Future protocol modules should preserve the existing layering:

1. `gs.core` owns execution, errors, and general facilities.
2. `gs.coro` provides coroutine scheduling and non-blocking synchronization.
3. Protocol modules own parsing, generation, connection behavior, clients, and
   servers without redefining the core runtime.
4. `gs.utils` supplies optional application-level services rather than becoming
   a protocol dependency.

New asynchronous APIs should remain consistent with Asio executors and
completion tokens. Synchronous convenience APIs should be explicit about
blocking and error behavior.

## Documentation policy

- The README presents stable project scope and implemented modules.
- Module documents describe APIs present in the source tree.
- Planned protocol work is labeled separately from the implemented baseline.
- Documentation code snippets should compile against the branch that contains
  them.

## Versioning

LibGS has not declared a 1.0 stability
milestone. Until then, protocol and utility APIs may evolve as module boundaries
and completion semantics are refined.
