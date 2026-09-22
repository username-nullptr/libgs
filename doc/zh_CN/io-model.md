# 执行与 I/O 模型

语言：[English](../en/io-model.md) | 简体中文

LibGS 遵循 Asio 的 executor、completion token、取消和对象安全模型。感知
executor 的类型提供 `executor_type`/`executor_t` 与 `get_executor()`。

## 运行时与完成处理

- `libgs::io_context()` 与 `libgs::get_executor()` 暴露进程级默认运行时；
  `libgs::exec()` 运行它，`libgs::exit()` 停止它。
- 多数感知 executor 的 API 也接受应用持有的 Asio executor。
- 异步发起函数不会内联调用完成处理器；即时错误也通过完成事件交付。
- completion token 关联的 executor、allocator、immediate executor 与
  cancellation slot 会继续传递。
- 为处理器绑定 executor 只改变完成位置，不会使底层 I/O 对象变为线程安全。
- 取消是异步的；借用数据必须一直有效到最终完成处理器执行。

## 所有权

- 除非重载明确说明复制或持有，否则 Buffer、view、引用和指针目标均为借用。
- Detached 协程应捕获自有状态；所有引用对象必须比协程存活更久。
- 只有在未完成操作已经完成，或已经取消并完成后，才能销毁 I/O 对象。
- 父对象返回的对象可能依赖父对象生命周期；以声明和模块指南为准。

## 并发

除非类型明确给出更强保证：

- 不同对象可以并发使用。
- 共享的有状态对象不安全。
- 同一逻辑对象的发起、处理器、取消、关闭和销毁都应串行化。

当多个线程执行 `io_context::run()` 时，在 strand 上构造对象，并让访问该对象的
所有处理器或协程运行在同一 strand：

```cpp
asio::io_context context;
auto strand = asio::make_strand(context);
libgs::websocket::client client(strand);

asio::co_spawn(strand, [&]() -> libgs::awaitable<void>
{
    auto stream = co_await client.open(
        "ws://127.0.0.1:8080/", libgs::use_awaitable);
    // 后续对 client 和 stream 的访问也留在此 strand。
}, libgs::detached);
```

只在发起调用外加互斥锁并不够：读、写、定时器、取消和关闭处理器仍可能并发执行。

## 操作限制

| 类型 | 同一对象的规则 |
| --- | --- |
| Core 调度 | `post` 和 `dispatch` 遵循传入的 executor |
| 协程同步 | 支持多个等待者；同步原语必须比等待者存活更久 |
| HTTP connection | 最多一个读和一个写；二者可以重叠 |
| HTTP client/server 状态 | 每个有状态对象的访问必须串行化 |
| WebSocket stream | 最多一个读族操作；一个读和一个写可重叠；写入进入有界队列 |
| Process | 最多一个 stdin 写、一个 stdout 读和一个 stderr 读 |
| Settings 与 INI I/O | 文件任务可使用工作 executor；可变对象状态仍需串行化 |

内部写队列或文件任务队列不代表其所属对象可以任意并发访问。
