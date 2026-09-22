# 应用工具

语言：[English](../en/utilities.md) | 简体中文

Target：`gs.utils`。启用 `LIBGS_BUILD_UTILITIES`，包含
`<libgs/utils.h>`，并链接 `gs.utils`。该模块依赖 `gs.coro`。

## 公共 API

| 头文件 | 功能 |
| --- | --- |
| `<libgs/utils/logger.h>` | 命名控制台与轮转文件日志 |
| `<libgs/utils/settings.h>` | 命名 INI 设置与变更信号 |
| `<libgs/utils/signal_slot.h>` | 同步、异步与背压 Signal |
| `<libgs/utils/observer.h>` | 由 executor 分派、按 ID 寻址的回调 |
| `<libgs/utils/modules.h>` | 按依赖顺序初始化应用模块 |
| `<libgs/utils/process.h>` | 子进程生命周期与标准流 I/O |
| `<libgs/utils/sbus.h>` | 可扩展发布/订阅与 Topic 缓存 |

`<libgs/utils.h>` 聚合 logger、settings、modules 和 soft bus。Process、
signal/slot 与 observer 需要直接包含对应头文件。

## 日志与设置

`utils::logger` 管理命名控制台和文件 logger，配置路径、级别、轮转、时间戳和格式。
文件 sink 为异步写入；需要确认落盘时调用 `logger::flush()`。

`utils::settings` 把 `libgs::ini` 包装为命名实例，并发出 `changed` 与
`loaded` 信号。值访问使用 `get()`/`set()`，持久化细节通过 `ini()`
控制。

## Signal、Observer 与 Modules

`utils::signal<Signature>` 支持：

| 模式 | 交付方式 |
| --- | --- |
| `sync` | 在调用线程直接执行 slot |
| `async` | 在 executor 上排队执行 |
| `backpressure` | 在 executor 上排队并阻塞到交付完成 |

不要从负责执行交付的同一 executor 线程调用 backpressure。分派会持有值参数，但
view、指针、引用和 slot 捕获对象的生命周期仍由调用方负责。

`utils::observer` 按稳定对象 ID 路由索引回调，并在销毁时注销。
`utils::modules` 注册命名初始化器、按依赖排序，并支持同步或异步初始化。

## 进程

`utils::process` 与 `utils::wprocess` 提供 start/run、join、detach、terminate、
kill、取消、超时、工作目录/环境、单实例锁，以及 stdin/stdout/stderr I/O。

I/O 未完成时保持 Process 存活；生命周期调用与 I/O 发起必须串行化，每个标准流方向
最多保留一个活动操作。

## 软总线

软总线将带类型的发布/订阅/缓存 API 与传输层分离：

| 传输 | 类型 | 可用条件 |
| --- | --- | --- |
| 进程内 | `local_interface`、`local_subscriber`、`local_cache` | 始终可用 |
| UDP 多播 | `udp_interface`、`udp_subscriber`、`udp_cache` | `LIBGS_BUILD_UTILITIES_SBUS_UDP=ON` |
| 自定义 | `basic_subscriber<Interface>`、`cache<Subscriber>`、`publish<Interface>()` | 用户实现 |

`LIBGS_UTILS_SBUS_DEFAULT_INTERFACE` 为未限定 API 选择 `local`（默认）或
`udp`。依赖固定传输的代码应显式写出类型。

UDP 传输带版本、分片、限速，并限制来源跟踪、重组和回调交付队列；支持进程、LAN 与
路由多播范围。它是 best-effort：过载保护可能丢弃流量，也不提供认证、加密或可靠
重放。网络策略与 ACL 应在 LibGS 外配置。

`utils::thread_pool()` 返回模块共享的 Asio thread pool。需要隔离或确定关闭顺序时
应使用自有 executor。

通用所有权和并发规则见[执行与 I/O 模型](io-model.md)。

## 示例

[Utilities 示例索引](../../examples/README.md#utilities)包含日志、设置、Signal、
Observer、Modules、Process、本地/UDP 总线与自定义传输。
