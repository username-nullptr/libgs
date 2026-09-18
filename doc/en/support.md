# Support Matrix

Language: English | [简体中文](../zh_CN/support.md)

This page states the boundaries of the shipped code. Module-specific behavior
and limits are documented in the corresponding module guide.

## Language and build integration

| Area | Supported | Outside the provided integration |
| --- | --- | --- |
| C++ | C++20 with GCC 13+, Clang 17+, or MSVC 19.30+ | Older compilers and language modes |
| Library form | Shared or static libraries | Header-only consumption |
| Source-tree CMake | Module targets through `add_subdirectory` | — |
| Install tree | Headers and enabled module libraries | CMake package config and imported targets |
| Event runtime | Process-wide default `asio::io_context` or application-owned executors | A separate non-Asio execution model |

Public APIs are pre-1.0 and may change between releases.

## Platform-dependent facilities

| Facility | Availability |
| --- | --- |
| POSIX system helpers | Unix-like builds |
| Windows system helpers | Windows builds |
| TLS | Builds configured with OpenSSL |
| io_uring | Linux builds configured with liburing |
| libc++ and lld switches | Clang builds |
| LTO switch | GCC builds |

Individual operating-system facilities can report a platform error when the
underlying capability is unavailable. The API guide for each facility describes
its return and error model.

## Protocols

| Area | Supported | Not provided |
| --- | --- | --- |
| HTTP | HTTP/1.0 and HTTP/1.1 client, server, parsing, generation, files, sessions, redirects, cookies, and proxies | HTTP/2 and HTTP/3 transport |
| HTTPS | HTTP over an application-configured OpenSSL context | Automatic certificate policy owned by LibGS |
| HTTP compression | gzip when `LIBGS_HTTP_ZLIB_SUPPORT=ON` | Compression without zlib |
| WebSocket | RFC 6455 client, server, Upgrade, frames, messages, masking, fragmentation, control frames, subprotocols, and bounded writes | WebSocket over HTTP/2 or HTTP/3 |
| WebSocket compression | RFC 7692 `permessage-deflate` when WebSocket zlib support is enabled | Other WebSocket extensions |
| WebSocket proxies | HTTP forward/CONNECT and SOCKS5 paths exposed by the client API | A managed system-wide proxy service |

The WebSocket API manages one connection. Application-level supervision,
persistent cross-connection queues, reliable message replay, and business
routing remain application responsibilities.

## Concurrency contract

- Distinct I/O objects can run concurrently.
- Access to one stateful I/O object is serialized unless its module guide says
  otherwise.
- Coroutine synchronization objects support multiple waiters but must outlive
  them.
- Cancellation requests do not release borrowed buffers or state before the
  completion handler runs.

See [Execution and I/O model](io-model.md) for the complete per-module rules.

## Module dependency boundary

Core is the base module. Coroutines depends on Core; HTTP and Utilities depend
on Coroutines; WebSocket depends on HTTP. Utilities has no protocol-module
dependency.
