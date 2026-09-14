# LibGS 文档

语言：[English](../en/README.md) | 简体中文

## 从这里开始

| 文档 | 内容 |
| --- | --- |
| [快速入门](getting-started.md) | 环境要求、配置、构建、安装与链接 |
| [核心模块](core.md) | 运行时、调度、数据类型、算法、容器与系统 API |
| [协程](coroutines.md) | 可等待操作与非阻塞同步 |
| [HTTP](http.md) | HTTP 协议、客户端、服务端、文件、会话、TLS、gzip 与代理 |
| [WebSocket](websocket.md) | RFC 6455 客户端、服务端、Stream、Upgrade、压缩与代理 |
| [应用工具](utilities.md) | 日志、设置、信号、观察者、模块、进程与软总线 |
| [路线图](roadmap.md) | 当前边界与规划方向 |

[示例指南](../../examples/README.md)把功能映射到可单独构建的程序。API
以 [`libgs/`](../../libgs) 下的公共头文件为准。

## 模块结构

| 模块 | 公共依赖 |
| --- | --- |
| `gs.core` | — |
| `gs.coro` | `gs.core` |
| `gs.http` | `gs.coro` |
| `gs.websocket` | `gs.http` |
| `gs.utils` | `gs.coro` |

每个模块都有聚合头文件；只使用单项功能时，建议直接包含更小的头文件。
`<libgs.h>` 根据生成的配置头包含构建时启用的模块。

## API 约定

- 多数异步操作遵循 Asio completion token 约定。
- 传入 `libgs::use_awaitable` 获得协程结果；具体声明允许时，也可使用回调或
  `libgs::detached`。
- 部分 API 默认同步执行。抛出式接口用 `std::system_error` 报告 I/O 错误；
  error-code 重载不抛异常。
- 异步 I/O 通常借用缓冲区直到操作完成，除非接口明确说明会复制数据。
- 接受 executor 的类型可以运行在应用自己的上下文，而不必使用进程级默认上下文。

LibGS 尚未到达 1.0，公共接口仍可能变化。
