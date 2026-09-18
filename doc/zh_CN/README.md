# LibGS 文档

语言：[English](../en/README.md) | 简体中文

这些文档只描述当前公共接口。先按任务找到入口，再把模块文档作为对应 API 的使用
指南。

## 入门与配置

| 任务 | 文档 |
| --- | --- |
| 完成第一次构建并运行程序 | [快速入门](getting-started.md) |
| 选择模块、功能和外部依赖 | [构建与配置](build.md) |
| 正确使用 executor、completion token、取消与 strand | [执行与 I/O 模型](io-model.md) |
| 确认协议、平台和集成边界 | [支持范围](support.md) |

## 模块指南

| 模块 | Target | 指南 | 聚合头文件 |
| --- | --- | --- | --- |
| Core | `gs.core` | [运行时与通用设施](core.md) | `<libgs/core.h>` |
| Coroutines | `gs.coro` | [可等待同步](coroutines.md) | `<libgs/coro.h>` |
| HTTP | `gs.http` | [HTTP/1.x 客户端与服务端](http.md) | `<libgs/http.h>` |
| WebSocket | `gs.websocket` | [RFC 6455 客户端、服务端与 Stream](websocket.md) | `<libgs/websocket.h>` |
| Utilities | `gs.utils` | [应用层服务](utilities.md) | `<libgs/utils.h>` |

模块依赖方向如下：

```text
gs.core
├── gs.coro
│   ├── gs.http
│   │   └── gs.websocket
│   └── gs.utils
```

使用 `add_subdirectory` 集成源码树时，只需链接应用直接使用的最高层模块，其公共
依赖会自动传递。

## 示例与验证

- [示例指南](../../examples/README.md)把功能映射到可单独构建的程序。
- [测试指南](../../test/README.md)说明功能、压力、Fuzz、性能与 Sanitizer 构建。
- [`libgs/`](../../libgs) 下的公共头文件是精确签名和重载范围的最终依据。

## 通用 API 约定

- 异步操作遵循 Asio completion token 约定。
- 传入 `libgs::use_awaitable` 获得协程结果；具体声明允许时，也可以使用回调或
  `libgs::detached`。
- 提供同步形式的 API 通过 `std::system_error` 抛出错误，或通过显式的
  `std::error_code&` 重载返回错误。
- Buffer 和其他借用参数通常由调用方持有到操作完成。
- Executor-aware 对象可以使用应用自己的上下文，不要求使用进程级默认上下文。

完整的生命周期和并发规则见[执行与 I/O 模型](io-model.md)。
