# WebSocket 模块设计

本文定义 `libgs::websocket` 的实现边界和可观察行为。除明确标记为“后续扩展”的内容外，均视为基础版本的实现约束。

## 1. 目标与范围

基础版本实现 RFC 6455 的 HTTP/1.1 WebSocket：

- client/server opening handshake；
- text、binary、continuation、Ping、Pong 和 Close；
- message fragmentation、客户端 masking 和完整协议校验；
- UTF-8、Close code、长度、RSV、opcode 和状态转换校验；
- 同步、异步、取消、超时、背压和优雅关闭；
- 独立 WS/WSS 服务，以及与现有 HTTP/HTTPS 服务混合部署；
- extension header 的通用解析和结构化表达。

基础版本不实现：

- HTTP/2、HTTP/3 extended CONNECT；
- extension 协商和帧变换 codec；
- WebSocket 层的代理配置、自动 keepalive、业务重连或消息路由。

WebSocket 复用 HTTP 模块完成 TCP/TLS 建连、Cookie、HTTP/1.1 报文处理和监听。升级成功后连接脱离 HTTP connection pool 和 HTTP keep-alive 生命周期，由 `websocket::stream` 独占。

## 2. 分层、依赖与所有权

### 2.1 对象职责

| 对象 | 职责 | 生命周期边界 |
| --- | --- | --- |
| `websocket::stream` | 升级后的消息 I/O、控制帧、状态机和关闭 | 独占一个已升级 connection |
| `websocket::client` | 持有 HTTP/1.1 client，发起 opening handshake | 只管理尚未完成的 open |
| `websocket::server` | 持有 HTTP server，监听、匹配、握手并交付 stream | 不保存已交付 stream |
| `websocket::open()` | 在调用者提供的 HTTP/1.1 client 上发起升级 | 不取得 HTTP client 所有权 |
| `websocket::upgrade()` | 在 HTTP service context 上完成服务端升级 | 成功后从 context 取走 connection |

依赖是单向的：

```text
core::url
    ↑
http protocol / connection
    ↑
websocket handshake adapter
    ↑
websocket stream / client / server
```

HTTP 模块不识别 `ws/wss`，frame codec 不依赖 socket、executor、HTTP client/server 或连接池。

### 2.2 Connection 移交

成功升级时 adapter 必须按以下顺序处理：

1. 完成并验证 opening handshake；
2. 取得 HTTP parser 尚未消费的 pending data；
3. 从 reply lease 或 service context 中取走 connection；
4. 用 connection、pending data、角色和协商结果构造 stream。

移交完成后 HTTP 层不得再次读写或回收该连接。任何失败路径都不得移交 connection；已经部分写出无效 101 或发生升级后 framing 错误的连接必须关闭，不能回到连接池。

## 3. 公共数据模型

所有公开名称直接位于 `libgs::websocket`。`protocol/` 只是源码目录，不增加 namespace。

`stream_config`、`client_config`、`connect_request`、`upgrade_options` 和 `server_config` 是可复制的配置快照；调用者直接设置字段。stream 在构造/adopt 时固定配置，open/accept/upgrade 在 operation initiation 时复制本次配置。

主要值类型约定如下：

- URL 使用 `libgs::url`；明确的协议文本使用 `std::string`；
- payload 使用 Asio buffer；
- 长度、状态码和超时使用明确数值或 chrono 类型；
- `extension_parameter::value` 使用 `std::optional<std::string>`：空 optional 表示无等号参数，字符串表示 token 或解引号后的 quoted-string；
- extension 参数中的数字含义由具体 extension codec 转换，基础协议层不提供数字类型。

配置边界统一为：

- `max_frame_size == 0`、`max_message_size == 0`：不增加实现上限，仅保留 RFC 可表示范围；
- `read_buffer_size == 0`：配置非法，返回 `std::errc::invalid_argument`；
- `max_queued_write_bytes == 0` 或 `max_queued_write_operations == 0`：不允许等待队列，但空闲时仍允许一个 active operation；
- `write_fragment_size == 0`：不自动拆分 outgoing message；
- duration 小于或等于零：deadline 已到达，不表示禁用超时。

`server::set_config()` 只替换未来操作使用的快照：无显式 options 的 accept 在 initiation 时取得 `default_upgrade`，无显式 options 的 handler 在注册时取得它；已挂起 accept、已排队 request 和已注册 handler 保留各自已有配置，缩小队列上限不驱逐已有项。公开配置结构体具有直接字段，后续改变布局需要遵循项目的 ABI/version 策略。

## 4. Operation、executor 与参数生命周期

### 4.1 Completion token

公开 I/O 遵循 libgs/Asio 约定：

- 默认 token 或 `use_sync`：同步执行，失败抛 `std::system_error`；
- `error_code&`：同步执行，不抛异常；
- callback：completion signature 以 `error_code` 开头；
- `use_awaitable`、`use_future` 等 throwing token：观察结果时抛出错误；
- `redirect_error`：把错误写入外部 `error_code`；
- 异步 operation 传播 associated executor、allocator 和 cancellation slot。

能够在 initiation 阶段发现的错误不启动底层 I/O。同步调用立即返回/抛出；异步调用仍通过 associated executor 完成，不能在 initiating function 内直接调用 handler。

构造对象时发现的无效配置抛出携带相应 `error_code` 的 `std::system_error`；由 operation 传入的无效配置通过该 operation 的同步/异步结果报告。

`sys_expected` 只用于 parser、generator、handshake codec 等无 I/O 算法及内部同步核心，不作为 stream/client/server I/O 的公开 completion value。

### 4.2 Detached 与借用

| Operation | 是否允许 detached | 结果观察方式 |
| --- | --- | --- |
| `write()` | 是 | `wait_written()` |
| `close()` | 是 | `wait_closed()` |
| `read()`、`wait_ctrl()` | 否 | 自身返回数据 |
| `ping()`、`pong()` | 否 | 无独立结果屏障 |
| `open()`、`accept()`、`upgrade()` | 否 | 成功结果包含 stream 所有权 |
| `wait_written()`、`wait_closed()` | 否 | 自身即观察操作 |

普通异步 operation 借用输入参数直到 completion。`write(detached)` 没有调用者可见的参数存活边界，因此必须在 initiation 返回前复制 payload。

所有返回 `size_t` 的写 operation 只统计用户 payload bytes，不包含 frame header 或 masking key。空 payload 成功返回零；部分写失败时只统计已经交给 transport 的 payload bytes。

### 4.3 Executor 与 deadline

返回 stream 所有权的 operation 在失败时也必须返回绑定到当前 executor 的 idle stream/result，不能假设 `basic_stream<Exec>` 能由全局 executor 默认构造。

异步 deadline 可以取消底层 I/O 并严格限制 operation。同步路径不创建隐藏线程，只能在阻塞步骤前后检查 deadline；某次同步 DNS/connect/read/write 可能超过 deadline。需要严格 wall-clock timeout 时应使用异步 token。

## 5. URL 与 HTTP transport 边界

### 5.1 协议归属

`libgs::url` 是通用 URL 描述，只负责保存和解析 scheme、authority、显式端口、编码 path/query 和 fragment。默认端口及特定协议是否合法由协议模块判断。

WebSocket client/open 在任何网络 I/O 前检查：

- URL 有效且 host 非空；
- scheme 可以是 `ws`、`wss`、`http` 或 `https`；
- URL 不包含 fragment。

`http` 和 `https` 是输入别名，必须在其他验证、安全策略及网络 I/O
之前分别规范化为 `ws` 和 `wss`。无效 URL、空 host 或 fragment 返回
`std::errc::invalid_argument`；其他 scheme 返回
`std::errc::protocol_not_supported`。

HTTP client 的公共边界仍只接受 `http/https`。WebSocket adapter 保留逻辑 WS URL，只为调用 HTTP client 创建临时 transport URL：

```text
ws  → http   默认端口 80
wss → https  默认端口 443
```

映射和规范化副本必须保留 host、显式端口和编码 path/query。
WebSocket resource-name 由 encoded path 加可选 encoded query 构成，至少包含
`/`，且永远不包含 fragment。diagnostics、redirect 基址及对外可见
endpoint 始终保持规范化后的 `ws/wss`。

### 5.2 代理与路由

WebSocket API 不提供 `proxy` 字段，也不生成或解释代理认证。owned client 使用内部 HTTP client 的 connector；自由 `open()` 使用调用者提供的 HTTP client。系统路由、代理或其他特殊建连策略由 HTTP client/connector 或运行环境负责，WebSocket 只继承最终建连行为。

### 5.3 Redirect

adapter 自己处理 301、302、303、307 和 308，不启用 HTTP client 的通用自动
redirect。`Location` 相对于当前逻辑 WS URL 解析，然后重新执行完整 WS URL
校验；绝对 `http/https` 目标同样先规范化为 `ws/wss`。收到可跟随的
redirect 但剩余次数为零时返回 `errc::redirect_limit_exceeded`。

每一跳都必须：

- 生成新的 `Sec-WebSocket-Key` 和 Host；
- 按新 endpoint 查询 Cookie jar；
- 跨 origin 时移除 origin-bound `Authorization` 和显式 Cookie；
- 重新执行 TLS、subprotocol 和 extension 响应校验。

origin 由 scheme、规范化 host 和有效端口组成。默认禁止 `wss → ws`；仅在 `allow_insecure_redirects` 为 true 时允许。`max_redirects == 0` 表示不跟随 redirect，超过上限返回 `errc::redirect_limit_exceeded`。

## 6. Client 与 opening handshake

### 6.1 入口

`websocket::client` 拥有一个 HTTP/1.1 client；每次 `client.open()` 返回独立 stream。自由 `websocket::open(http_client, ...)` 用于 HTTP/WebSocket 混合客户端，不拥有传入的 HTTP client。

`connect_request` 保存 endpoint、本次 HTTP request options、可选 stream 配置、可选 handshake timeout、subprotocol offer、extension 占位和 redirect policy。owned client 在请求未覆盖时使用 `client_config::stream` 和 `client_config::handshake_timeout`；自由 `open()` 没有 client-level 默认值，未覆盖项使用 WebSocket 默认配置。

### 6.2 请求与验证

每一跳固定发送 HTTP/1.1 GET，并使用操作系统密码学安全随机源生成新的 16-byte nonce 和 `Sec-WebSocket-Key`。随机源失败时在相关字节上线前结束 operation；不得回退到时间戳、计数器、普通 PRNG 或固定种子。

`request_options` 可以提供目标服务器认证、Cookie、Origin 和普通应用 header，但不能覆盖 `Host`、`Connection`、`Upgrade`、`Proxy-Authorization` 或任何 `Sec-WebSocket-*`。冲突配置在 I/O 前以 `errc::invalid_upgrade` 失败。

final response 必须满足：

- status 为 101；
- `Connection` 包含 `Upgrade` token；
- `Upgrade` ASCII case-insensitive 等于 `websocket`；
- `Sec-WebSocket-Accept` 与本次 key 匹配；
- 选择的 subprotocol 属于 client offer；
- 基础版本没有选择 extension。

非 101 final response 返回 `errc::handshake_rejected`。收到 101 但 WebSocket 校验失败时，连接必须从 lease 中移除并关闭，不能回池。

### 6.3 Timeout、取消与 diagnostics

request 中的 `handshake_timeout` 优先于 owned client 的默认值；自由 `open()` 使用 WebSocket 默认值。deadline 从 operation initiation 开始，覆盖所有 redirect hop 的 DNS、connect、TLS、request write 和 final response header read，不因 redirect 重置。

deadline 小于或等于零时不触碰网络。deadline 到达后取消当前底层 operation；参与过本次 handshake 的 connection 必须关闭，最终返回 `asio::error::timed_out`。`client::cancel()` 只影响尚未完成的 open，不影响已经交付的 stream。

`basic_open_diagnostics` 是可选的 caller-owned out-parameter：

- initiation 时清空旧 reply；
- endpoint 保存当前或最终逻辑 WS URL；
- 只保留最终 response，不保留中间 redirect；
- 调用者保证其存活到 completion，且不得并发复用同一对象。

成功 101 时 reply 只保留响应元数据，connection lease 已转移给 stream。非 101 时 diagnostics reply 保留 lease，调用者可读取拒绝 body；没有 diagnostics 时，未消费 reply 随内部对象释放并关闭。收到无效 101 时 diagnostics 仍可保留 response metadata，但不能保留可复用 lease。无论成功失败，open 都不允许 detached。

## 7. Server 与 upgrade

### 7.1 混合服务

`websocket::upgrade(context, options)` 用于现有 HTTP route。它完整验证 opening request，写出 final response，成功后取得 pending data 并调用 `hand_over_connection()`。`is_upgrade_request()` 只识别 Upgrade 意图，不替代完整校验。

`upgrade_options::handshake_timeout` 从 `upgrade()` initiation，或 owned server 把 request 与 accept/handler 配对时开始；它覆盖 policy 前后检查、响应生成和完整 response write，不包含 HTTP request header 的读取时间及 pending queue 等待时间。

timeout 小于或等于零时不写 101 或 rejection，立即关闭对应 HTTP connection 并返回 `asio::error::timed_out`。response write 期间到期时取消写入并关闭连接。同步 validator/selector 不能被强制中断，adapter 必须在每个 callback 返回后重新检查 deadline。

### 7.2 Owned server 与交付模式

`websocket::server` 内部拥有 HTTP server。底层 transport/listener 配置通过 `http_server()` 管理；WebSocket config 只包含 upgrade policy 和 pending-handshake 队列。

server 有两种互斥的 stream 交付模式：

- 主动 `accept()` 返回 `basic_accept_result`；
- `on_connection()`/`on_default()` 把同样的结果交给 `awaitable<void>` handler。

第一次 accept 或第一次注册 handler 固定模式。callback 模式再调用 accept，以 `std::errc::operation_not_supported` 完成；accept 模式再注册 handler，抛 `std::logic_error`。

`basic_accept_result` 包含独占 stream、`request_info` 快照和 `upgrade_result`。快照拥有 method、version、target、path、headers、query/path 参数及本地/远端 endpoint，不借用 HTTP context。server 不跟踪已交付 stream。

### 7.3 Pending handshake

主动 accept 模式维护两个 FIFO：pending accept 和已经读完 HTTP request、尚未发送 101 的 pending handshake。两者按顺序配对，每个 request 使用对应 accept 自带的 `upgrade_options`。系统永远不排队无人持有的 upgraded stream。

- `max_pending_handshakes` 限制等待配对的 request 数；
- `pending_handshake_timeout` 只限制配对前等待；
- 上限为零或 timeout 非正时，没有现成 accept waiter 的 request 立即以 503 拒绝；
- 已有 accept waiter、可以立即配对的 request 不进入 pending queue；
- 配对后停止 pending timer，并启动该 accept 的 handshake timeout；排队时间不从新 deadline 扣除。

队列满/超时的 503 使用 request 入队时 server 配置快照中的默认 handshake timeout 作为 response write deadline。

单个 request 的格式错误、policy rejection、callback exception、response I/O error 或 timeout 只结束该 HTTP request。已经配对的 accept 保留原 options 和 FIFO 位置，继续等待下一次成功升级；accept 只因显式取消、server stop 或 server 级不可恢复错误结束。

### 7.4 服务端校验与响应

服务端按以下顺序处理：

1. HTTP method/version 和 Upgrade 必需 header；
2. WebSocket version、key、subprotocol token 和 extension 语法；
3. `request_validator`；
4. `origin_validator`；
5. subprotocol 选择；
6. 基础版 extension policy；
7. 写出 final response；
8. 仅在成功 101 后移交 connection。

默认 subprotocol 算法按 client offer 顺序选择第一个服务端支持项。自定义 selector 替代默认算法，但结果仍必须同时属于 client offer 和 server capability；`require_subprotocol` 为 true 且无共同协议时以 400 拒绝。

标准失败响应：

- opening request 格式错误：400；
- WebSocket version 不是 13：426，并包含 `Sec-WebSocket-Version: 13`；
- pending queue 满或超时：503；
- policy rejection：使用 `upgrade_rejection` 中的非 101 final response。

`upgrade_validation_result == std::nullopt` 表示接受。rejection 自有 status、headers 和 body；合法 status 为 200--599 范围内的非 101 final status，非法值改为 500。失败 response 完整写出后 operation 才完成；若 response 写失败、只写出一部分或 deadline 到达，则关闭连接。

失败完成错误保持具体原因：格式错误为 `errc::invalid_upgrade`，version 非 13 为 `errc::unsupported_version`，明确 policy rejection 为 `errc::handshake_rejected`，subprotocol/extension policy 分别为对应 unsupported error，response I/O 则保留 transport error。若非 101 response 已完整写出且允许 HTTP keep-alive，混合模式仍由原 service context 管理连接，owned server 也继续遵循其 HTTP session 生命周期；任何失败都不能产生 WebSocket stream。

protocol-owned 101 headers 不能被 `response_headers` 覆盖；rejection 的协议 header 和 body framing header 同样由 adapter 管理。validator/selector 是同步用户代码，adapter 必须捕获异常并用 `exception_error()` 映射；可以安全写响应时尝试发送 500，但 operation 始终保留先出现的 callback error。需要异步鉴权时，应用应先在 HTTP route 中完成，再调用 `upgrade()`。owned server 仅在成功升级后调用 connection handler；失败交给 HTTP/WebSocket 错误路径和已注册的 service error handler。

`server::cancel()` 和 `stop()` 只影响 listener、pending accept 和未完成 handshake，不影响已交付 stream。

## 8. Stream

### 8.1 状态与 adopt

`connection_state` 含义：

| 状态 | 含义 |
| --- | --- |
| `idle` | 尚未 adopt connection |
| `open` | 可以收发 message/control frame |
| `closing` | Close handshake 已开始，不再接受普通数据写 |
| `closed` | 正常关闭或显式 shutdown 已完成 |
| `failed` | 协议、framing 或 transport 出现不可恢复错误 |

`adopt()` 只接受已经完成并验证 HTTP Upgrade 的 connection。它设置角色、pending data、subprotocol 和协商 extension，不解析 URL，也不发送 opening handshake。重复 adopt、非法预读状态或不匹配 executor 必须失败；基础版拒绝非空 `negotiated_extensions`。

### 8.2 Receive engine

stream 不启动后台读取。只有 `read()` 和正在完成关闭握手的 `close()` 从 transport 读取。每个 stream 始终只有一个 receive engine 和一个底层 read。

`read<Buffer>()` 返回一个完整 text/binary message；TCP 分段、多个 frame、continuation 和控制帧都由内部 parser/assembler 消化。零长度 message 合法。控制帧不作为普通 message 返回。

同一时刻最多有一个 message waiter 和一个 control waiter；重叠发起同类 operation 返回 `std::errc::operation_in_progress`。`close()` 接管接收方向前先以 `errc::closing` 完成已有 read/control waiter。

`read_buffer_size` 只决定内部 transport read block。`max_frame_size` 限制单帧，`max_message_size` 限制全部 continuation 累计后的完整消息；两者独立。消息超过上限返回 `errc::message_too_big`，并在 framing 仍可信时发送 Close 1009。

所有 incoming/outgoing text message 必须验证 UTF-8，该行为不可关闭。incoming 非法 UTF-8 执行 protocol failure 并使用 Close 1007；outgoing 非法 UTF-8 在任何 frame 上线前返回 `protocol_errc::invalid_utf8`。

`closed` 状态再次 read 返回 `asio::error::eof` 和空 message。`closing` 状态的新 read 返回 `errc::closing`；`failed` 状态始终返回保存的首个错误，不能伪装成 EOF。

### 8.3 Ping、Pong 与控制事件

`wait_ctrl()` 观察 receive engine 解析出的下一个 Ping/Pong，不启动独立 transport read，也不返回 Close。仅有 wait_ctrl pending 时不会推进网络；应用必须同时保持 read，或由 close 接管 receive engine。

`automatic_pong == true` 时，收到 Ping 后先把相同 payload 的 Pong 放入协议保留槽，再完成 control waiter。若已有未发送 automatic Pong，可用最新 Ping payload 覆盖，保持常量空间。

`automatic_pong == false` 时，stream 保存最新一个未交付 Ping，应用通过 wait_ctrl 取得后必须尽快发送相同 payload 的 Pong。无 waiter 时历史 Pong 可以丢弃。`ping()` 只等待 Ping frame 写出，不隐式等待远端 Pong。

stream 不实现周期 Ping、idle timeout 或 Pong deadline；应用通过 timer、cancellation slot、ping 和 wait_ctrl 组合 keepalive。

### 8.4 Send engine、分片与背压

每个 stream 只有一个 send engine 和一个底层 write。数据 write 以 message 为排队单位，线上不会交错两个 message。`write_fragment_size` 控制自动分片；控制帧只能在 frame 边界插入。

等待队列限制只统计尚未成为 active 的 operation：

- `max_queued_write_bytes` 统计等待中的 data payload；
- `max_queued_write_operations` 统计等待中的 data、Ping 和 Pong；
- 两个非零上限必须同时满足，容量运算不得整数溢出；
- 超限返回 `errc::write_queue_full`；
- Close 和 automatic Pong 使用独立、有界的协议保留槽，不占应用队列。

frame scheduler 在边界处选择下一帧，优先级为：可发送的 Close、automatic Pong、应用 Ping/Pong、下一段 data fragment。Close 不参与公平轮转；其他 control 与 data 同时等待时，最多连续发送一个 control frame，然后发送一个 data fragment。

operation 在任何 payload 上线前取消，不破坏连接。frame 已部分写出后发生取消或错误会破坏 framing，stream 进入 `failed` 并关闭 transport。

`wait_written()` 是快照屏障：等待调用前已接受的全部 data write，并报告其中第一个尚未被较早屏障观察的错误，包括 detached write 的错误。

### 8.5 Close handshake

本地主动 `close()`：

1. 切换为 `closing` 并拒绝新 data write；
2. 尽量排空 close 前已接受的 data write；
3. 发送一个 Close frame；
4. 继续读取直到收到 peer Close；
5. 关闭 transport 并返回 `close_info`。

`close_timeout` 从首次 close initiation 开始，覆盖排队、发送和等待 peer 的全过程。非正 timeout 不发送 Close，立即关闭 transport 并返回 `asio::error::timed_out`；到期时同样强制关闭，`clean == false`。

重复 close 加入同一关闭状态机，后续参数被忽略，不重复发送 Close。关闭完成后再次 close 或 wait_closed 立即成功返回保存的结果。`wait_closed()` 只是观察器，不自行读取 transport。

peer 先发送 Close 时，stream 启动同一个 close deadline，不再开始新 message；未开始的排队 write 以 `std::errc::broken_pipe` 和零 payload bytes 完成，已经开始的当前 message 可在 deadline 内完成，然后发送 Close response。观察到 peer Close 后的新 write 返回 `broken_pipe`；本端先 close 时返回 `errc::closing`；终态返回 `errc::closed`。

`close_info::code` 为空表示空 Close，或 transport 在没有 Close frame 时结束。只有双向 Close handshake 完成且没有 protocol/transport error 时 `clean` 才为 true。

### 8.6 Cancel、shutdown 与并发

`cancel()` 请求取消当前 operation，不发送 Close，也不以关闭连接为目的；若取消发生在部分 frame 写出后，仍进入 `failed`。

`shutdown()` 立即取消所有 operation 并关闭整个 transport，不发送 Close，也不是 TCP 半关闭。未完成 I/O 返回 `asio::error::operation_aborted`，wait_closed 观察到 `clean == false`。析构使用 shutdown 语义，不等待优雅关闭。

stream 遵循 Asio shared-object-unsafe 约定。跨线程调用必须由 strand 或外部锁串行化。允许在同一串行 executor 上重叠发起 write；状态查询和修改也必须遵循同一规则。取消 wait_ctrl 只取消观察者，不取消 parser 或由 read 驱动的 transport read；取消 read 可以取消由它驱动的底层 read，但不能销毁 parser state。client/server 的配置、启动、取消和状态查询同样需要串行调用或外部同步。

### 8.7 内部实现边界

stream 的私有实现按状态所有权拆分：

- `frame_builder` 负责 outgoing message/control frame 的校验、分片、masking
  和 wire buffer 构造，不拥有 connection 或 executor；
- `receive_buffer` 负责 pending/transport 输入缓存、增量 frame parsing 和完整
  message assembly，不启动 transport read，也不保存 operation waiter；
- operation/waiter 按方向拆在 `send_operations.h`、`receive_operations.h` 和
  `close_operations.h`，接收方向不会间接依赖出站 frame builder；
- `send_engine` 独占 frame builder、发送队列、wire scheduler、write waiter 和
  automatic Pong/Close 等待发送槽；
- `receive_engine` 独占 receive buffer、read operation/waiter、control observer，
  并通过同一个 frame-event pump 为普通 read 与 Close 接管流程提供输入；
- `stream::impl` 是 transport 与生命周期 facade，独占 connection I/O，只通过窄
  host contract 协调两侧 engine、Close handshake、protocol failure 和连接终态。

新增协议算法应优先放入无 transport/executor 依赖的组件；单向 operation 状态属于
对应 engine，只有跨 receive、send 和 Close 生命周期的状态转换留在 `stream::impl`。
完整组件图、文件职责和代码跳转路径见 `detail/stream/README.md`。

## 9. 协议 codec

### 9.1 文件职责

```text
websocket/
├── error.h
└── protocol/
    ├── types.h       frame、close、extension、codec config、protocol_errc
    ├── parser.h      增量 frame parser 和 Close payload decoder
    ├── generator.h   frame header、Close payload 和 masking
    └── handshake.h   opening handshake header codec
```

这些组件不拥有 socket 或 executor，可独立测试和 fuzz。

### 9.2 Frame parser/generator

`frame_header` 是规范化元数据，不用 C++ bitfield 映射网络字节。`frame_codec_config::local_role` 决定 masking 方向：client 发出的 frame 必须 mask，server 发出的 frame 不得 mask。

`frame_parser` 一次 parse 最多推进一个 frame，并跨调用保存 partial header、payload 进度、fragmentation state、failed 状态和首错。它必须校验：

- canonical payload length 和 64-bit length 最高位；
- opcode、RSV 及角色对应的 masking；
- control frame 的 FIN 和 125-byte 上限；
- continuation/message fragmentation 顺序。

parse result 的 payload 是当前输入上的借用 slice，仍保持 wire masking；调用者必须在释放或移动输入前处理。`header_ready` 和 `frame_finished` 是单次 transition event。`reset()` 清除 partial frame、失败和 fragmentation 状态。

`encode_frame_header()` 执行同等的 outbound 校验。generator 不产生 masking key；stream 为每个 client frame 生成独立、不可预测的 32-bit key。`apply_mask()` 支持 payload offset，`mask_copy()` 在 destination 不足时返回 `no_buffer_space` 且不修改 destination。

Close payload helper 必须接受空 payload、拒绝单字节 payload、验证可上线状态码和 reason UTF-8，并保证编码后不超过 125 bytes。可上线状态码包括本模块枚举出的已分配协议状态码和 `[3000, 5000)` 的注册/私有状态码；1005、1006、1015 等只用于本地表达的保留值不得上线。

### 9.3 Handshake codec

handshake codec 负责：

- client key、GUID/SHA-1/Base64 accept key；
- HTTP/1.1、GET、version 13 和必需 Upgrade headers；
- header token 的 ASCII case-insensitive 匹配；
- key Base64 格式和解码后 16-byte 长度；
- subprotocol/extension header 语法；
- 服务端 subprotocol 必须属于 client offer。

它不处理 URL、redirect、I/O、deadline、Cookie 或 connection 移交，这些属于 client/server adapter。

## 10. Extension 边界

基础层把 extension header 解析为有序 `extension`/`extension_parameter`：名称和值使用字符串，vector 保留顺序和重复项；quoted-string 解转义后保存语义值，不保留原始引号，解转义结果仍必须满足 token 语法。generator 可以规范化为 token 输出，无法合法表示的值必须拒绝。通用 parser 只验证 RFC 6455 语法，参数唯一性、数值范围和组合规则由具体 extension 判断。

基础版本行为固定为：

- 可以解析合法 client extension offer，server 默认忽略；
- `connect_request::extensions` 非空时在 I/O 前返回 `errc::unsupported_extension`；
- server 的 `supported_extensions` 非空或设置 `extension_selector` 时拒绝本地配置；
- client 收到任何 extension response 时失败并关闭已参与 handshake 的连接；
- adopt 收到非空 negotiated extensions 时失败；
- `allowed_rsv` 始终为空，任何 RSV 位均为 protocol error。

后续 extension 实现必须把四个层次作为一个能力单元：

| 层次 | 责任 |
| --- | --- |
| Wire descriptor | offer/response 的名称和参数 |
| Negotiation policy | client 校验、server 选择及参数约束 |
| Codec capability | RSV 所有权、收发变换和组合顺序 |
| Session state | 每连接、每消息、fragment 和 context takeover 状态 |

selector 只能选择已经安装 codec capability 的 offer；成功协商必须同时创建逐连接 codec state 并检查 RSV 冲突。裸 `extension` 描述不能单独启用扩展。首个计划实现为 RFC 7692 permessage-deflate，但其 codec 生命周期稳定前不冻结额外的公开 provider vtable。

## 11. 错误与失败语义

错误保留来源 category：

| 来源 | 类型 | 示例 |
| --- | --- | --- |
| Frame wire violation | `protocol_errc` | mask、opcode、RSV、长度、UTF-8、fragmentation |
| WebSocket operation | `websocket::errc` | handshake、closing、closed、queue full |
| 系统/transport | Asio 或 `std::errc` | timeout、aborted、broken pipe、TLS/I/O |

opening handshake 主要映射：

- 非 101 final response：`handshake_rejected`；
- method/version/header 组合非法：`invalid_upgrade`；
- WebSocket version 非 13：`unsupported_version`；
- Accept 不匹配：`invalid_accept_key`；
- subprotocol 非 offer 成员：`unsupported_subprotocol`；
- 基础版 extension 配置/响应：`unsupported_extension`；
- redirect 超限/安全降级：`redirect_limit_exceeded` / `insecure_redirect`。

发生 frame protocol error 时统一执行 `fail_connection`：

1. 保存首个协议错误并停止处理、交付后续入站数据；
2. 若 handshake 已完成、尚未发送 Close 且 outbound framing 可信，只排队一个对应 Close；
3. Close 写完、写失败或 deadline 到达后关闭 transport；
4. 无法可靠发送 Close 时直接关闭；
5. stream 最终进入 `failed`，所有观察者得到保存的首错。

protocol failure 不转换成正常 EOF。所有不可恢复路径遵循首错优先，不能把 stream 恢复为 open；重新连接必须创建新 stream。

## 12. 实现与验证顺序

按依赖顺序实现：

1. error category、协议类型和基础校验；
2. frame generator、mask 和 Close payload codec；
3. incremental frame parser；
4. opening handshake codec；
5. 无 extension 的 stream 状态机；
6. `open()` 和 owned client；
7. `upgrade()` 和 owned server；
8. 独立设计 extension capability registry；
9. RFC 7692 permessage-deflate。

基础版本至少覆盖以下验证矩阵：

- generator/parser round-trip、partial header/payload、多个 frame 和 fuzz；
- canonical length、masking 方向、RSV/opcode/control frame 边界；
- fragmentation 中插入控制帧；
- UTF-8、Close code/reason 和 125-byte 边界；
- handshake 正反例、重复/大小写 header、非法 token/key；
- ws/wss URL、encoded resource-name、fragment、redirect、Cookie 和 TLS downgrade；
- message 边界、空 message、自动分片和 partial-write 计数；
- write queue 上限、scheduler 公平性、detached barrier 和取消竞态；
- automatic/manual Pong、read/wait_ctrl 共存和 receive ownership；
- local/peer/simultaneous/repeated Close、timeout、EOF、shutdown；
- pending accept/handshake FIFO、容量、超时、503 和 cancellation；
- 400/426/policy rejection/500、keep-alive 与 connection handover；
- 不能由全局 executor 默认构造的自定义 `Exec` 错误路径；
- 真实 socket 的 client/server 互操作。

公共头文件能够编译不代表基础版本完成；只有无 extension 路径通过以上协议与互操作测试后，才视为具备可发布实现。
