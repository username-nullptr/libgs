# 快速入门

语言：[English](../en/getting-started.md) | 简体中文

## 环境要求

| 组件 | 最低版本 |
| --- | --- |
| CMake | 3.15 |
| C++ | C++20 |
| GCC | 13 |
| Clang | 17 |
| MSVC | 19.30 / Visual Studio 2022 |

仓库已包含 standalone Asio、spdlog 与 nlohmann/json。可选功能所需的系统依赖
会在 CMake 配置阶段查找。

## 配置模块

所有模块默认启用。模块开关及依赖关系如下：

| 开关 | 默认值 | 作用 |
| --- | :---: | --- |
| `LIBGS_BUILD_CORO` | `ON` | 构建 `gs.coro`；HTTP 与 Utilities 依赖它 |
| `LIBGS_BUILD_HTTP` | `ON` | 构建 `gs.http`；依赖 Coroutines |
| `LIBGS_BUILD_WEBSOCKET` | `ON` | 构建 `gs.websocket`；依赖 HTTP |
| `LIBGS_BUILD_UTILITIES` | `ON` | 构建 `gs.utils`；依赖 Coroutines |
| `LIBGS_BUILD_STATIC` | `OFF`* | 构建静态库而不是共享库 |
| `LIBGS_ADD_LIBRARY_VERSION` | `ON` | 在共享库文件名中加入版本 |
| `LIBGS_BUILD_EXAMPLES` | `OFF` | 构建示例程序 |

功能开关：

| 开关 | 平台/依赖 | 作用 |
| --- | --- | --- |
| `LIBGS_OPENSSL_SUPPORT` | OpenSSL | HTTPS、WSS 与 TLS 连接类型 |
| `LIBGS_HTTP_ZLIB_SUPPORT` | zlib | HTTP gzip 与 WebSocket 压缩支持 |
| `LIBGS_WEBSOCKET_ZLIB_SUPPORT` | zlib | 不启用 HTTP gzip 时单独启用 WebSocket `permessage-deflate` |
| `LIBGS_IO_URING_SUPPORT` | Linux 与 liburing | 使用 Asio io_uring 后端 |
| `LIBGS_USE_LIBCXX` | Clang | 使用 libc++ |
| `LIBGS_USE_LLD` | Clang | 使用 lld 链接 |
| `LIBGS_ENABLE_LTO` | GCC | 启用链接时优化 |

启用 HTTP zlib 后，WebSocket 会继承 zlib 支持，不再提供单独的 WebSocket
开关。请通过 CMake 配置功能，不要在应用代码中自行定义生成的功能宏。

Windows 上的 GNU 工具链如果没有共享 `libstdc++`，会默认改为静态构建。测试、
Sanitizer 与 Fuzzer 开关见[测试指南](../../test/README.md)。

## 构建与安装

```sh
git clone https://gitee.com/jin-xiaoqiang/libgs.git
cd libgs

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/path/to/libgs-install
cmake --build build --parallel
cmake --install build
```

多配置生成器需要在构建和安装时指定配置：

```sh
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=C:/libgs
cmake --build build --config Release --parallel
cmake --install build --config Release
```

库文件生成在 `build/output/bin` 或 `build/output/lib`；示例程序生成在
`build/output/examples`。

## 链接应用

通过 `add_subdirectory` 使用源码树时，只需链接应用直接使用的最高层模块，
它的公共依赖会自动传递：

```cmake
add_subdirectory(path/to/libgs)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE gs.http)
```

安装树目前不提供 CMake package config。使用已安装版本时，需要显式设置头文件
目录并查找所需库：

```cmake
find_path(LIBGS_INCLUDE_DIR NAMES libgs.h REQUIRED)
find_library(LIBGS_CORE_LIBRARY NAMES gs.core REQUIRED)
find_library(LIBGS_CORO_LIBRARY NAMES gs.coro REQUIRED)
find_library(LIBGS_HTTP_LIBRARY NAMES gs.http REQUIRED)

add_executable(my_app main.cpp)
target_compile_features(my_app PRIVATE cxx_std_20)
target_include_directories(my_app PRIVATE "${LIBGS_INCLUDE_DIR}")
target_link_libraries(my_app PRIVATE
  "${LIBGS_HTTP_LIBRARY}"
  "${LIBGS_CORO_LIBRARY}"
  "${LIBGS_CORE_LIBRARY}"
)
```

## 运行默认上下文

```cpp
#include <libgs/core/execution.h>

int main()
{
    libgs::post([]
    {
        libgs::exit();
    });

    return libgs::exec();
}
```

`libgs::exec()` 会阻塞并运行默认 `asio::io_context`。`libgs::exit(code)`
停止该上下文，并设置 `exec()` 的返回值。带 executor 的重载可使用应用自己的
上下文。

## 完成方式与错误

具体接口支持哪些 completion token，以其声明为准。常见形式如下：

| 形式 | 用法 |
| --- | --- |
| 协程 | 传入 `libgs::use_awaitable`，然后 `co_await` |
| 回调 | 传入签名兼容的 completion handler |
| 分离执行 | 接口支持时传入 `libgs::detached` |
| 同步、抛异常 | 使用默认同步 token |
| 同步、不抛异常 | 传入 `std::error_code&` |

## 后续阅读

- [核心运行时与通用设施](core.md)
- [协程支持](coroutines.md)
- [HTTP 客户端与服务端](http.md)
- [WebSocket](websocket.md)
- [应用工具](utilities.md)
