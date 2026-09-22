# Execution and I/O Model

Language: English | [简体中文](../zh_CN/io-model.md)

LibGS follows Asio's executor, completion-token, cancellation, and object-safety
model. Executor-aware types expose `executor_type`/`executor_t` and
`get_executor()`.

## Runtime and completion

- `libgs::io_context()` and `libgs::get_executor()` expose the process-wide
  default runtime. `libgs::exec()` runs it; `libgs::exit()` stops it.
- Most executor-aware APIs also accept an application-owned Asio executor.
- Asynchronous initiating functions do not invoke their completion handlers
  inline. Immediate failures are delivered as completions.
- A token's associated executor, allocator, immediate executor, and cancellation
  slot are propagated.
- Binding a handler to an executor changes where completion runs; it does not
  make the underlying I/O object thread-safe.
- Cancellation is asynchronous. Borrowed data remains required until the final
  completion runs.

## Ownership

- Buffers, views, references, and pointer targets are borrowed unless an
  overload explicitly says it copies or owns them.
- Detached coroutines should capture owned state. Every referenced object must
  outlive the coroutine.
- Destroy an I/O object only after its outstanding operations have completed or
  been cancelled and completed.
- Objects returned by a parent object can depend on that parent's lifetime; use
  the declaration and module guide to confirm ownership.

## Concurrency

Unless a type documents a stronger contract:

- Distinct objects may be used concurrently.
- Shared stateful objects are unsafe.
- Serialize initiation, handlers, cancellation, close, and destruction for one
  logical object.

With multiple `io_context::run()` threads, construct the object on a strand and
run every handler or coroutine that accesses it on the same strand:

```cpp
asio::io_context context;
auto strand = asio::make_strand(context);
libgs::websocket::client client(strand);

asio::co_spawn(strand, [&]() -> libgs::awaitable<void>
{
    auto stream = co_await client.open(
        "ws://127.0.0.1:8080/", libgs::use_awaitable);
    // Keep later access to client and stream on this strand.
}, libgs::detached);
```

A mutex around initiating calls alone is not enough: read, write, timer, cancel,
and close handlers may still run concurrently.

## Operation limits

| Type | Rule for one object |
| --- | --- |
| Core scheduling | `post` and `dispatch` follow the supplied executor |
| Coroutine synchronization | Multiple waiters are supported; the primitive must outlive them |
| HTTP connection | At most one read and one write may be active; one of each may overlap |
| HTTP client/server state | Serialize access to each stateful object |
| WebSocket stream | One read-family operation may be active; a read and write may overlap; writes use a bounded queue |
| Process | At most one stdin write, one stdout read, and one stderr read may be active |
| Settings and INI I/O | File work may use a worker executor; serialize mutable object state |

Queueing writes or internal file jobs does not imply arbitrary thread-safe access
to the containing object.
