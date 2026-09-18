# Utilities Examples

[Examples index](../README.md) · [Utilities guide](../../doc/en/utilities.md)

Utilities examples require `LIBGS_BUILD_UTILITIES=ON`. They use only local
process, filesystem, thread, and in-process messaging facilities.

## Programs

| Program | Demonstrates | Arguments and side effects |
| --- | --- | --- |
| `logger` | Default and named logger configuration | Optional log directory; defaults to `./logs` and writes log files there |
| `settings` | Named settings, change signals, load-or-create, and sync | Optional INI path; defaults to `libgs-example-settings.ini` |
| `signal_slot` | Free-function and lambda slots | No arguments |
| `observer` | ID-addressed observer callback and lifecycle | No arguments |
| `modules` | Multi-source module registration, dependency graph, and ordered initialization | No arguments |
| `process` | Child start, stdout read, join, and exit code | Runs `cmd.exe /C echo` on Windows or `/bin/echo` on POSIX |
| `soft_bus_local` | Built-in local publish/subscribe and topic cache | No arguments |
| `soft_bus_transport` | A custom transport implementing the soft-bus interface | No arguments |

The `modules` executable is assembled from `modules/main.cpp`,
`foundation.cpp`, `cache.cpp`, and `api.cpp`.

## Run without persistent output

```sh
./build/output/examples/utils/signal_slot
./build/output/examples/utils/observer
./build/output/examples/utils/modules
./build/output/examples/utils/process
./build/output/examples/utils/soft_bus_local
./build/output/examples/utils/soft_bus_transport
```

## Control generated files

Pass explicit paths for logging and settings:

```sh
./build/output/examples/utils/logger /tmp/libgs-example-logs
./build/output/examples/utils/settings /tmp/libgs-example-settings.ini
```

`soft_bus_local` uses the built-in in-process transport.
`soft_bus_transport` shows the interface boundary for plugging in DDS, IPC, or
another transport; it does not start an external transport service.
