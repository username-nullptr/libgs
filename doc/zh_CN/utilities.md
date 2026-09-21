# 应用工具

语言：[English](../en/utilities.md) | 简体中文

`gs.utils` 在 `gs.coro` 之上提供应用层服务。

## 公共结构

| 头文件 | 功能 |
| --- | --- |
| `<libgs/utils/logger.h>` | 命名控制台与轮转文件日志 |
| `<libgs/utils/settings.h>` | 命名的 INI 设置与变更信号 |
| `<libgs/utils/signal_slot.h>` | 同步、异步与背压信号投递 |
| `<libgs/utils/observer.h>` | 通过 executor 投递的 ID 寻址回调 |
| `<libgs/utils/modules.h>` | 按依赖顺序初始化应用模块 |
| `<libgs/utils/process.h>` | 子进程生命周期与标准流 I/O |
| `<libgs/utils/sbus.h>` | 可扩展发布/订阅与 Topic 缓存 |

`<libgs/utils.h>` 聚合 logger、settings、modules 与 soft bus。process、
signal/slot、observer 需要直接包含各自头文件。

## 功能

### 日志

`utils::logger` 管理命名实例以及 console、daily、warning、error、critical sink。
配置控制路径、级别、轮转、时间戳与格式。日志宏会捕获 `std::source_location`。
文件 sink 异步工作；需要确认写入完成时调用 `logger::flush()`。

### 设置

`utils::settings` 把 `libgs::ini` 包装成命名实例，并发出 `changed` 与
`loaded` 信号。使用 `get()`/`set()` 读写值，需要直接控制持久化时使用 `ini()`。

### 信号与观察者

`utils::signal<Signature>` 支持三种投递模式：

| 模式 | 行为 |
| --- | --- |
| `sync` | 直接调用 slot 并阻塞调用方 |
| `async` | 把 slot 提交到 executor |
| `backpressure` | 提交到 executor，并阻塞直到投递完成 |

不要从负责执行投递的同一 executor 线程使用 backpressure。Signal 会持有投递
状态和按值参数，但不会延长 view、指针、引用或 slot 捕获对象的生命周期。

`utils::observer` 通过稳定对象 ID 路由带索引的回调签名。Observer 采用共享
所有权，并在析构时注销。

### 模块

`utils::modules` 注册命名初始化器，并按父/子依赖排序。初始化器可以接收命令行
参数，返回 `void` 或 `bool`。`do_init()` 支持同步与异步完成；`sprint()` 输出
依赖图。

### 进程

`utils::process` 与 `utils::wprocess` 支持 start/run、join、detach、terminate、
kill、取消、超时、环境变量/工作目录、单实例锁以及标准输入/输出/错误流。异步
I/O 未完成时必须保持进程对象存活，并检查返回错误以处理平台差异。按照
[执行与 I/O 模型](io-model.md)，生命周期调用必须与 I/O 发起串行化；同一
process 同时最多保留一个 stdin 写、一个 stdout 读和一个 stderr 读。

### 软总线

软总线把带类型的 publish/subscribe/cache API 与传输实现分离。
`sbus::local_interface`、`local_subscriber` 与 `local_cache` 提供进程内传输。
启用 `LIBGS_BUILD_UTILITIES_SBUS_UDP` 后，`udp_interface`、`udp_subscriber`
与 `udp_cache` 提供基于 Asio 的 UDP 多播传输；发布时显式使用
`publish<udp_interface>()`。

`LIBGS_UTILS_SBUS_DEFAULT_INTERFACE` 可为未指定接口的 `publish()`、
`subscribe()` 以及 `default_interface`、`default_subscriber` 别名选择
`local`（默认值）或 `udp`。需要固定传输方式的代码应显式使用对应的 interface
或 subscriber 类型，以免该构建选项改变其行为。

UDP 传输启用本机回送；`udp_interface::config_t` 默认使用管理域多播组
`239.255.71.83`、端口 `57183` 以及 `msg_range::process`。进程范围报文携带
进程令牌并使用 TTL 0；`lan` 使用 TTL 1，`internet` 使用 TTL 64。接收范围
按层级表示可接受的最远发布范围，而其他进程发布的 `process` 报文始终会被
拒绝。`internet` 仍
依赖支持多播路由的网络，并不表示公网必然可达。

带版本的线协议支持最长 4 KiB 的 Topic 和分片后的最大 16 MiB 负载。每个接口
在解析前执行全局及按源地址的报文数/字节数令牌桶限流，最多跟踪 1024 个源
地址，忽略没有 Topic 订阅者或全局订阅者的报文，并把单一源地址限制为最多
8 个未完成消息及 16 MiB 重组内存；周期定时器会按 5 秒超时清理未完成消息。
每个接口的全局上限为 128 个未完成消息和 64 MiB。完整消息进入最多
256 条/32 MiB 的有界队列，用户回调在独立投递
线程上执行，因此慢回调不会无限期阻塞 socket 收包。可通过
`udp_interface::statistics()` 查看非法报文、限流丢弃、重组淘汰、投递队列
丢弃和投递进度。多播端点、范围、socket 缓冲区、消息大小、速率/突发阈值、
来源跟踪、重组限制/超时及投递队列限制均直接在 `config_t` 中配置。
`set_config()` 影响后续静态发布以及之后默认构造的接收接口；已经存在的接收
接口保留构造时的配置快照。需要同时使用不同接收策略时，可向构造函数显式
传入 `config_t`。

UDP 是尽力而为传输，过载保护会主动丢包。它不提供认证或加密；LAN 和路由
模式只应在合适的可信网络中使用，并应在不可信边界配置来源及多播 ACL。
`basic_subscriber<Interface>`、`cache<Subscriber>` 与
`publish<Interface>()` 仍可用于接入自定义传输。

`utils::thread_pool()` 返回 Utilities 模块共享的 Asio 线程池。需要隔离或明确
关闭顺序时，应使用应用自己持有的 executor。

## 示例

- [日志](../../examples/utils/logger.cpp)
- [设置](../../examples/utils/settings.cpp)
- [信号](../../examples/utils/signal_slot.cpp)
- [观察者](../../examples/utils/observer.cpp)
- [模块](../../examples/utils/modules)
- [进程](../../examples/utils/process.cpp)
- [本地软总线](../../examples/utils/soft_bus_local.cpp)
- [UDP 软总线](../../examples/utils/soft_bus_udp.cpp)
- [自定义软总线传输](../../examples/utils/soft_bus_transport.cpp)
