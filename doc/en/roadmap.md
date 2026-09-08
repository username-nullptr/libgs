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
| WebSocket | In development | Protocol framing, handshake, and basic stream send I/O exist; read state machine is in progress |

## Planned WebSocket support

WebSocket is the next explicitly planned protocol expansion. The intended
direction is to build it as a protocol module that integrates with existing
LibGS execution, connection, TLS, and completion-token conventions.

Expected design areas include:

- client and server roles;
- `ws://` and `wss://` transports;
- HTTP Upgrade validation and connection handover;
- text, binary, continuation, ping, pong, and close frames;
- fragmented-message assembly;
- callback and coroutine completion styles;
- cancellation, timeout, queue, and backpressure behavior; and
- clear ownership of buffers and handed-over connections.

Current landed conventions: `read()` returns complete data messages only; Ping/Pong
events are retained while reading continues; after Close, subsequent reads return
`0 + eof`. Concurrent operations in one direction remain the caller's
responsibility. The protocol parser is incremental and borrows the input buffer;
payload copying is limited to message aggregation where ownership is required.

## Existing preparation for upgrades

The HTTP server already exposes a protocol-neutral ownership boundary through
`service_context::hand_over_connection()`. It allows an accepted HTTP Upgrade
connection and pending input to leave the normal HTTP request lifecycle. This
reduces coupling between the HTTP parser and a future upgraded protocol, but it
does not parse or generate WebSocket frames.

## Architectural direction

Future protocol modules should preserve the existing layering:

1. `gs.core` owns execution, errors, and general facilities.
2. Header-only coroutine helpers provide non-blocking synchronization.
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
- This roadmap is the only place where planned but unimplemented protocol work
  is presented as planned scope.
- Documentation code snippets should compile against the branch that contains
  them.

## Versioning

LibGS is currently at `0.11.1-dev` and has not declared a 1.0 stability
milestone. Until then, protocol and utility APIs may evolve as module boundaries
and completion semantics are refined.
