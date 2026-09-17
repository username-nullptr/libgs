# Asio 兼容的 I/O 模型

语言：[English](../en/io-model.md) | 简体中文

LibGS 使用与 Asio 相同的执行和并发模型。绑定 executor 的公共类型提供
`executor_type` 与 `get_executor()`；旧名称 `executor_t` 作为兼容别名保留。

## 完成处理器

- 异步发起函数不会在调用栈内直接调用 completion handler。参数校验或状态错误也会
  作为异步完成投递。
- 自定义组合操作可使用 `post_completion(exec, handler, args...)` 实现这一规则；它会
  保留 handler 的 associated immediate executor 与 allocator，语义与
  `asio::async_immediate` 一致。
- 使用默认 immediate executor 时，完成会进入队列。与 Asio 相同，显式定制
  associated immediate executor 的 token 可以选择更强的即时执行语义。
- completion token 关联的 executor、allocator 与 cancellation slot 会继续传播；
  handler 没有关联 executor 时，`get_executor()` 返回的 executor 作为 fallback。
- 为 handler 绑定其他 executor 只改变用户完成回调的运行位置，不会让 I/O 对象的
  并发访问自动变得安全。
- 除非重载明确说明会持有或复制数据，否则 buffer 等借用参数必须存活到操作完成。
- 取消是异步过程；收到取消完成之前销毁被引用的 buffer 或状态仍然是错误用法。

## 线程安全与 strand

除非类型另有明确说明，采用 Asio 的标准契约：

- 不同对象：安全。
- 共享同一对象：不安全。

访问同一个逻辑 I/O 对象时，需要串行化发起调用和相关 handler。当一个
`io_context` 由多个线程运行时，让对象以及访问它的协程或 handler 使用同一个
strand：

```cpp
asio::io_context context;
auto strand = asio::make_strand(context);
libgs::websocket::client client(strand);

asio::co_spawn(strand, [&]() -> libgs::awaitable<void>
{
    auto stream = co_await client.open(
        "ws://127.0.0.1:8080/", libgs::use_awaitable);
    // 对 client 和 stream 的访问始终留在这个 strand 上。
}, libgs::detached);
```

只在发起调用外包一层 mutex 不够：read、write、timer、cancel 和 close 的 handler
仍可能并发执行。析构也必须与对象访问串行化，并且不能和未完成操作竞争。

## 各模块的操作规则

| 模块/类型 | 同一对象上的并发操作 |
| --- | --- |
| Core 调度 | `post` 与 `dispatch` 遵循传入的 executor。`basic_ini` 是 shared-unsafe I/O 对象；文件任务在内部串行化。 |
| 协程同步 | mutex、semaphore、condition variable 支持并发 wait/notify；对象必须比等待者存活更久。 |
| HTTP connection | 同时最多一个读和一个写；一个读与一个写可以重叠。高层 request/reply 状态必须串行访问。 |
| HTTP client/server | 不同 client、connection、request context 可以并行；每个有状态对象仍遵循 shared-unsafe 规则。 |
| WebSocket stream | `read`、`read_frame`、`consume` 同时只能有一个活动操作；一个读与一个写可以重叠；消息写通过有界队列串行化。 |
| Process | 每个标准流方向同时最多一个操作：一个 stdin 写、一个 stdout 读、一个 stderr 读；生命周期操作必须与 I/O 发起串行化。 |
| Settings 与应用 I/O | 文件任务可以使用工作 executor，完成回调使用 token 的 associated executor；可变对象状态仍需串行访问。 |

WebSocket 写队列是相对于 `asio::async_write` 的有意高层扩展；后者通常要求调用方
避免重叠写。队列上限提供背压，但不代表 stream 对象可以被任意多线程共享调用。
