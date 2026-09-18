# 快速入门

语言：[English](../en/getting-started.md) | 简体中文

下面先构建默认启用的 Core 与 Coroutines，运行仓库内示例，再把 LibGS 链接到
应用。

## 1. 构建仓库

LibGS 需要 CMake 3.15、C++20，以及 GCC 13+、Clang 17+ 或 MSVC 19.30+。
仓库已包含默认依赖，不需要另外下载。

```sh
git clone https://gitee.com/jin-xiaoqiang/libgs.git
cd libgs

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DLIBGS_BUILD_EXAMPLES=ON
cmake --build build --parallel
```

默认配置构建 `gs.core`、`gs.coro` 及其示例。运行 Core 的执行示例：

```sh
./build/output/examples/core/execution
```

Visual Studio 等多配置生成器还需要在构建命令中添加 `--config Release`。

## 2. 运行最小应用

```cpp
#include <libgs/core/execution.h>

#include <chrono>

int main()
{
    using namespace std::chrono_literals;

    libgs::post(1s, []
    {
        libgs::exit();
    });

    return libgs::exec();
}
```

`libgs::post()` 把任务提交到进程级默认上下文；`libgs::exec()` 运行该上下文并
阻塞，直到 `libgs::exit()` 停止它。Executor-aware 重载也可以使用应用自己持有的
Asio 上下文。

把 LibGS 放在同一个 CMake 构建中时，链接程序使用的模块：

```cmake
cmake_minimum_required(VERSION 3.15)
project(my_app LANGUAGES CXX)

add_subdirectory(path/to/libgs)

add_executable(my_app main.cpp)
target_compile_features(my_app PRIVATE cxx_std_20)
target_link_libraries(my_app PRIVATE gs.core)
```

## 3. 启用应用模块

HTTP、WebSocket 与 Utilities 需要显式启用。包含全部模块及其示例的构建命令为：

```sh
cmake -S . -B build-full -DCMAKE_BUILD_TYPE=Release \
  -DLIBGS_BUILD_HTTP=ON \
  -DLIBGS_BUILD_WEBSOCKET=ON \
  -DLIBGS_BUILD_UTILITIES=ON \
  -DLIBGS_BUILD_EXAMPLES=ON
cmake --build build-full --parallel
```

只需链接应用直接使用的最高层模块。例如链接 `gs.websocket` 时，它的 HTTP、
Coroutine 与 Core 公共依赖也会传递给应用。

## 继续阅读

- [构建与配置](build.md)：模块开关、OpenSSL、zlib、io_uring、安装与应用集成。
- [执行与 I/O 模型](io-model.md)：completion token、Buffer 生命周期、取消、
  strand 与单对象并发。
- [示例](../../examples/README.md)：按模块组织的可运行程序。
- [模块指南](README.md#模块指南)：当前公共能力。
