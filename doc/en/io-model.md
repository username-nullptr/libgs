# Asio-compatible I/O model

Language: English | [简体中文](../zh_CN/io-model.md)

LibGS uses the same execution and concurrency model as Asio. Executor-aware
public types expose `executor_type` and `get_executor()`; the older
`executor_t` name remains as a compatibility alias.

## Completion handlers

- An asynchronous initiating function does not invoke its completion handler
  inline. Immediate validation and state errors are posted as completions.
- `post_completion(exec, handler, args...)` exposes this rule to custom
  composed operations while retaining the handler's associated immediate
  executor and allocator, matching `asio::async_immediate`.
- With the default immediate executor this completion is queued. As in Asio, a
  token that explicitly customizes its immediate executor may choose stronger
  immediate-execution behavior.
- The executor, allocator, and cancellation slot associated with a completion
  token are propagated. `get_executor()` supplies the fallback executor when a
  handler has no associated executor.
- Binding a different executor changes where the user completion runs. It does
  not make concurrent access to the I/O object safe.
- Buffers and other borrowed arguments must remain valid until completion,
  unless an overload explicitly documents that it owns or copies them.
- Cancellation is asynchronous. Destroying referenced buffers or state before
  the cancellation completion is still invalid.

## Thread safety and strands

Unless a type explicitly states otherwise, use the standard Asio contract:

- Distinct objects: safe.
- Shared objects: unsafe.

For a shared logical I/O object, serialize initiating calls and its handlers.
When an `io_context` has multiple runner threads, use the same strand for the
object and the coroutines or handlers that access it:

```cpp
asio::io_context context;
auto strand = asio::make_strand(context);
libgs::websocket::client client(strand);

asio::co_spawn(strand, [&]() -> libgs::awaitable<void>
{
    auto stream = co_await client.open(
        "ws://127.0.0.1:8080/", libgs::use_awaitable);
    // All access to client and stream stays on this strand.
}, libgs::detached);
```

A mutex around initiating calls alone is insufficient: read, write, timer,
cancel, and close handlers may still execute concurrently. Destruction must be
serialized with access to the object and must not race outstanding operations.

## Per-module operation rules

| Module/type | Concurrent operations on one object |
| --- | --- |
| Core scheduling | `post` and `dispatch` follow the supplied executor. `basic_ini` is a shared-unsafe I/O object; its file jobs are serialized internally. |
| Coroutine synchronization | Mutexes, semaphores, and condition variables support concurrent wait/notify operations; they must outlive their waiters. |
| HTTP connection | At most one read and one write may be outstanding. One read and one write may overlap. Higher-level request/reply state must be accessed serially. |
| HTTP client/server | Independent clients, connections, and request contexts may run concurrently. Access to each individual stateful object follows the shared-unsafe rule. |
| WebSocket stream | One `read`, `read_frame`, or `consume` operation may be active. A read and a write may overlap. Message writes are serialized through the bounded write queue. |
| Process | At most one operation per standard stream direction should be outstanding: one stdin write, one stdout read, and one stderr read. Lifecycle operations must be serialized with I/O initiation. |
| Settings and application I/O | File work may use worker executors, while completion uses the token's associated executor. Access to mutable object state must still be serialized. |

The WebSocket write queue is an intentional high-level extension over
`asio::async_write`, which normally requires the caller to avoid overlapping
writes. Queue limits provide backpressure; they do not imply arbitrary
thread-safe access to the stream object.
