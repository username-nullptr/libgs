# Core Examples

[Examples index](../README.md) · [Core guide](../../doc/en/core.md)

Core examples are built by default when `LIBGS_BUILD_EXAMPLES=ON`. They are
offline and finish without another process.

## Programs

| Program | Demonstrates | Arguments and side effects |
| --- | --- | --- |
| `algorithms` | MIME lookup, SHA-1, UUID generation, and wildcard matching | No arguments |
| `app_paths` | Executable, working, home, and absolute paths | No arguments |
| `args_parser` | Grouped options, flags, version/help handling, and positional arguments | Accepts `-o/--output`, `-v/--verbose`, `--version`, `-h/--help`, and positional values |
| `dynamic_library` | Loading a shared library and resolving a typed symbol | Optional plugin path; otherwise loads the companion plugin beside the executable |
| `execution` | Dispatch, queued/delayed post, periodic timers, cancellation, and the default event loop | No arguments; runs for about one second |
| `ini` | Loading, updating, and syncing an INI file | Optional path; defaults to `libgs-example.ini` |
| `lock_free_queue` | Concurrent producer/consumer delivery | No arguments |
| `value` | Text/numeric conversions, formatting, and invalid conversion | No arguments |

`dynamic_library_plugin` is a shared-library fixture for `dynamic_library`,
not a standalone executable.

## Run

```sh
./build/output/examples/core/execution
./build/output/examples/core/algorithms
./build/output/examples/core/value
```

Exercise argument parsing:

```sh
./build/output/examples/core/args_parser \
  --output=result.txt --verbose input.dat
```

Choose the INI output path:

```sh
./build/output/examples/core/ini /tmp/libgs-example.ini
```

The dynamic-library target builds its plugin dependency automatically:

```sh
cmake --build build --target libgs.example.core.dynamic_library
./build/output/examples/core/dynamic_library
```
