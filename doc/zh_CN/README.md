# LibGS 文档

语言：[English](../en/README.md) | 简体中文

## 阅读入口

| 需求 | 文档 |
| --- | --- |
| 构建、选择模块或集成 LibGS | [构建与配置](build.md) |
| 了解执行器、完成令牌、生命周期和并发约束 | [执行与 I/O 模型](io-model.md) |
| 查找可运行程序 | [示例](../../examples/README.md) |
| 构建或筛选测试 | [测试](../../test/README.md) |

## 模块

| 模块 | Target | 聚合头文件 | 指南 |
| --- | --- | --- | --- |
| Core | `gs.core` | `<libgs/core.h>` | [核心模块](core.md) |
| Coroutines | `gs.coro` | `<libgs/coro.h>` | [协程](coroutines.md) |
| HTTP | `gs.http` | `<libgs/http.h>` | [HTTP](http.md) |
| WebSocket | `gs.websocket` | `<libgs/websocket.h>` | [WebSocket](websocket.md) |
| Utilities | `gs.utils` | `<libgs/utils.h>` | [应用工具](utilities.md) |

依赖方向：

```text
gs.core
└── gs.coro
    ├── gs.http
    │   └── gs.websocket
    └── gs.utils
```

只需链接应用直接使用的最高层模块，其公共依赖会通过 CMake 自动传递。

## API 约定

- 异步 API 使用 Asio completion token；声明允许时，传入
  `libgs::use_awaitable` 可获得协程结果。
- 同步 API 按声明通过 `std::system_error`、显式 `std::error_code&` 或
  expected 风格结果报告错误。
- 借用的 Buffer 和引用状态必须存活到异步操作完成。
- 有状态 I/O 对象不会自动保证线程安全；同一对象的访问通常应由 strand 串行化。
- [`libgs/`](../../libgs) 下的公共头文件定义精确的 API 范围。
