# 构建与配置

语言：[English](../en/build.md) | 简体中文

本页统一说明构建要求、CMake 开关、产物、安装和支持边界。

## 要求

| 组件 | 要求 |
| --- | --- |
| CMake | 3.15 或更高版本 |
| 语言 | C++20 |
| GCC | 13 或更高版本 |
| Clang | 17 或更高版本 |
| MSVC | 19.30 或更高版本（Visual Studio 2022+） |

仓库已包含 standalone Asio、spdlog 和 nlohmann/json。OpenSSL、zlib 与
liburing 只在启用对应功能时作为系统依赖。

## 模块选择

| 开关 | 默认值 | Target | 依赖 |
| --- | :---: | --- | --- |
| Core（始终构建） | ON | `gs.core` | — |
| `LIBGS_BUILD_CORO` | ON | `gs.coro` | Core |
| `LIBGS_BUILD_HTTP` | OFF | `gs.http` | Coroutines |
| `LIBGS_BUILD_WEBSOCKET` | OFF | `gs.websocket` | HTTP |
| `LIBGS_BUILD_UTILITIES` | OFF | `gs.utils` | Coroutines |

启用模块但关闭其依赖时，CMake 会拒绝配置。模块依赖公开传递，应用只需链接直接使用
的最高层模块。

默认构建：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

构建全部模块与示例：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DLIBGS_BUILD_HTTP=ON \
  -DLIBGS_BUILD_WEBSOCKET=ON \
  -DLIBGS_BUILD_UTILITIES=ON \
  -DLIBGS_BUILD_EXAMPLES=ON
cmake --build build --parallel
```

## 功能开关

| 开关 | 默认值 | 作用 |
| --- | :---: | --- |
| `LIBGS_BUILD_STATIC` | OFF* | 构建静态库而非共享库 |
| `LIBGS_ADD_LIBRARY_VERSION` | ON | 为共享库添加版本信息 |
| `LIBGS_OPENSSL_SUPPORT` | OFF | 启用 TLS、HTTPS 和 WSS；需要 OpenSSL |
| `LIBGS_HTTP_ZLIB_SUPPORT` | OFF | 启用 HTTP gzip；需要 zlib |
| `LIBGS_WEBSOCKET_ZLIB_SUPPORT` | OFF | 启用 WebSocket `permessage-deflate`；需要 zlib |
| `LIBGS_BUILD_UTILITIES_SBUS_UDP` | ON | 构建 UDP 软总线传输 |
| `LIBGS_UTILS_SBUS_DEFAULT_INTERFACE` | `local` | 为未限定的软总线 API 选择 `local` 或 `udp` |
| `LIBGS_IO_URING_SUPPORT` | OFF | 在 Linux 上使用 Asio io_uring；需要 liburing |
| `LIBGS_BUILD_EXAMPLES` | OFF | 构建已启用模块的示例 |

HTTP zlib 会在构建 WebSocket 时自动启用其 zlib 支持；只有关闭 HTTP zlib 时才提供
独立的 WebSocket 开关。

*Windows GNU 工具链找不到共享 `libstdc++-6.dll` 时，默认改为静态构建。*

工具链开关：

| 开关 | 适用范围 | 作用 |
| --- | --- | --- |
| `LIBGS_USE_LIBCXX` | Clang | 使用 libc++ 编译和链接 |
| `LIBGS_USE_LLD` | Clang | 使用 lld 链接 |
| `LIBGS_ENABLE_LTO` | GCC | 启用 LTO |
| `LIBGS_HEAVY_COMPILE_JOBS` | 测试/示例 | 限制 HTTP/WebSocket 并行编译数；`0` 表示不限 |
| `LIBGS_LOW_MEMORY_DEBUG_INFO` | GCC Debug 构建 | 使用 `-g1` 降低编译内存 |

测试专用开关见[测试](../../test/README.md)。

## 产物与安装

单配置构建的输出目录：

| 产物 | 路径 |
| --- | --- |
| 共享库与可执行文件 | `build/output/bin` |
| 静态库与导入库 | `build/output/lib` |
| 示例 | `build/output/examples/<module>` |
| 生成的配置头文件 | `build/output/config_include` |

多配置生成器可能增加配置子目录。

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/path/to/libgs-install
cmake --build build --parallel
cmake --install build
```

Visual Studio 等多配置生成器需要在构建和安装命令中添加
`--config Release`。

## 使用源码树

```cmake
cmake_minimum_required(VERSION 3.15)
project(my_app LANGUAGES CXX)

set(LIBGS_BUILD_HTTP ON CACHE BOOL "")
add_subdirectory(path/to/libgs)

add_executable(my_app main.cpp)
target_compile_features(my_app PRIVATE cxx_std_20)
target_link_libraries(my_app PRIVATE gs.http)
```

在 `add_subdirectory()` 前设置 LibGS 选项。链接 Target 会提供内置头文件和传递
模块/系统依赖。

## 使用安装树

将安装前缀加入 `CMAKE_PREFIX_PATH`，然后加载已安装的 package，并链接应用使用的
最高层模块：

```cmake
find_package(LibGS 0.16 CONFIG REQUIRED COMPONENTS http)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE LibGS::http)
```

例如，配置消费项目时传入 `-DCMAKE_PREFIX_PATH=/path/to/libgs-install`。可用组件名和
导入 Target 为 `core`、`coro`、`http`、`websocket` 和 `utils`；请求的组件必须在构建
LibGS 时已启用。package 同时提供旧目标名（`gs.core`、`gs.coro` 等）以兼容源码树
用法，并会恢复所需的可选系统依赖。

## 支持边界

- LibGS 以共享库或静态库构建，不是 header-only 库。
- 源码树和安装树均支持 Target 化 CMake 集成。
- 运行时基于 Asio，可使用进程级默认上下文或应用持有的 Asio executor。
- TLS 需要应用配置 OpenSSL context；LibGS 不负责证书策略。
- HTTP 支持 1.0 和 1.1，不支持 HTTP/2 或 HTTP/3。
- WebSocket 实现基于 HTTP/1.1 的 RFC 6455。
- 公共 API 尚未达到 1.0，版本间可能变化。
