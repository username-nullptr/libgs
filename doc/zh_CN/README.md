# LibGS 文档

语言：[English](../en/README.md) | 简体中文

LibGS 是一个面向异步应用与服务的模块化 C++20 基础库。项目按层次组织：
核心执行与通用工具层、面向协程的同步设施、协议模块，以及更高层的应用工具。

HTTP/HTTPS 和 HTTP/1.1 WebSocket 都是已实现的协议模块，但不是项目的边界。
协议层将持续扩展；WebSocket 已包含受限的 RFC 7692 压缩配置，更广泛的扩展及
基于新版 HTTP 的传输仍属于规划工作。

## 文档导航

| 文档 | 内容 |
| --- | --- |
| [快速入门](getting-started.md) | 环境要求、构建选项、安装、链接方式和 API 约定 |
| [核心模块](core.md) | 事件循环、任务调度、value、INI、算法、容器和系统工具 |
| [协程支持](coroutines.md) | 协程任务、同步原语、等待操作和执行器切换 |
| [HTTP](http.md) | HTTP 客户端、服务端、路由、会话、TLS、gzip 和完成方式 |
| [WebSocket](websocket.md) | RFC 6455 stream、客户端/服务端 API、Upgrade 集成、WS/WSS 和当前限制 |
| [应用工具](utilities.md) | 日志、设置、信号、观察者、模块、进程和可扩展软总线 |
| [项目路线图](roadmap.md) | 已实现范围、规划中的协议扩展和项目方向 |

## 模块概览

| CMake 目标 | 主要头文件 | 作用 |
| --- | --- | --- |
| `gs.core` | `<libgs/core.h>` 和 `<libgs/core/...>` | 运行时与通用基础设施 |
| `gs.core` | `<libgs/coro.h>` 和 `<libgs/coro/...>` | 基于核心运行时的纯头文件协程设施 |
| `gs.http` | `<libgs/http.h>` 和 `<libgs/http/...>` | HTTP 协议、客户端、服务端和可选 TLS 支持 |
| `gs.websocket` | `<libgs/websocket.h>` 和 `<libgs/websocket/...>` | HTTP/1.1 WebSocket 协议、客户端、服务端、stream、可选压缩和可选 WSS 支持 |
| `gs.utils` | `<libgs/utils.h>` 和 `<libgs/utils/...>` | 可复用的应用服务与工具 |

`<libgs.h>` 会包含当前启用的顶层模块头文件。当编译时间和依赖边界比较重要时，
建议直接包含具体头文件。部分工具 API（包括进程、信号和观察者类型）需要直接
包含各自的头文件，并未包含在 `<libgs/utils.h>` 聚合头中。

## 通用 API 约定

### 执行器与调度

默认运行时拥有一个进程级 `asio::io_context`，可通过 `libgs::io_context()`
和 `libgs::get_executor()` 访问。很多类型也可以使用兼容的外部执行器构造，
因此任务既可以运行在默认事件循环上，也可以交给应用自己管理的执行上下文。

### 完成方式

异步操作通常遵循 Asio 的 completion token 约定。根据具体操作，调用方可以使用：

- 回调函数；
- 在 C++20 协程中使用 `libgs::use_awaitable`；
- 使用 `libgs::detached` 发起无需等待结果的任务；
- 使用 error code token 进行非抛异常的同步错误处理；
- 使用部分客户端和工具 API 提供的默认同步形式。

每个操作接受的 token 和完成签名应以对应头文件中的声明为准。

### 错误处理

会抛异常的同步重载通过 `std::system_error` 报告 I/O 失败。非抛异常形式会接收
error code token，或返回 `sys_expected<T>`。异步回调的完成签名中包含错误码。

### API 状态

项目尚未达到 1.0 API 稳定里程碑，因此公共接口仍可能
调整。本文档始终将已实现功能与规划功能分开描述。

## 源码索引

- 公共头文件：[`libgs/`](../../libgs)
- 示例：[`examples/`](../../examples)
- 构建配置：[`CMakeLists.txt`](../../CMakeLists.txt)
- 第三方依赖来源：[`3rd_party/SOURCE.txt`](../../3rd_party/SOURCE.txt)
