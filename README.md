# A4 Buffer Pool Manager

Starter code for the buffer-pool programming assignment in IM110. The assignment writeup is `rec04.qmd` in the parent directory.

## Layout

```
recs04/
  include/        Public headers; do not change public signatures.
  src/            Implementations; the four student files contain TODOs.
  tests/          Catch2 visible tests.
  tools/          Demonstration binary for the flat-file page store.
  CMakeLists.txt  Build configuration; fetches fmt, CLI11, Catch2.
  CMakePresets.json
                  Debug, asan, ubsan, tsan presets.
```

## Recommended Environment: Dev Container

This repository provides a Dev Container (`.devcontainer/`) that installs a fixed C++ toolchain: Clang, CMake, Make, `clang-format`, `clang-tidy`, GDB, Git, `ccache`. Using it removes toolchain variance.

The repository also ships:

| File | Purpose |
|---|---|
| `.devcontainer/devcontainer.json` | Defines the development container and VS Code integration. |
| `.devcontainer/Dockerfile` | Installs the C++ toolchain. |
| `.vscode/extensions.json` | Recommends VS Code extensions for Dev Containers, C++, and CMake. |
| `.vscode/settings.json` | Enables formatting, static analysis, and CMake presets in VS Code. |
| `.clang-format` | Mechanical formatting style. |
| `.clang-tidy` | Static-analysis checks. |
| `CMakePresets.json` | Reproducible Debug, ASan, UBSan, TSan builds. |

### VS Code

1. Install Docker Desktop or a Docker-compatible runtime.
2. Install VS Code and the `ms-vscode-remote.remote-containers` extension ("Dev Containers" in the marketplace).
3. Open the `recs04/` directory in VS Code.
4. Accept "Reopen in Container", or run `Dev Containers: Reopen in Container` from the Command Palette.
5. Wait for the first build of the container image.
6. VS Code runs `cmake --preset debug` after container creation. If that step fails (transient network), re-run it manually in the integrated terminal.

Recommended VS Code extensions, also pre-listed in `.vscode/extensions.json`:

* `ms-vscode-remote.remote-containers` — Dev Containers.
* `ms-vscode.cpptools` — Microsoft C/C++.
* `ms-vscode.cmake-tools` — CMake integration.

Select the `debug` CMake preset from the CMake Tools status bar. The preset uses the `Unix Makefiles` generator, so the build backend is plain `make`.

### Other Editors

The Dev Container also runs from the command line via the `devcontainer` CLI:

```bash
devcontainer up --workspace-folder .
devcontainer exec --workspace-folder . cmake --preset debug
devcontainer exec --workspace-folder . cmake --build --preset debug
devcontainer exec --workspace-folder . ctest --preset debug --output-on-failure
```

Editor integration outside VS Code is editor-specific. Most editors with a language-server client can consume `build/debug/compile_commands.json`.

### Native Toolchain

A native install without Docker is acceptable: C++20 compiler, CMake, Make, Git, `clang-format`, `clang-tidy`, sanitizer runtimes. Environment differences are then your responsibility.

## Build and Test

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

CTest labels:

* `infra` — provided page stores, allocator, blocking queue. Should pass before any student code is written.
* `assignment` — `ArcReplacer`, `DiskScheduler`, page guards, BPM. Expected to fail until the TODOs are implemented.
* `perf` — informational benchmark, excluded from the default preset.

Run a single label:

```bash
ctest --preset debug -L infra
ctest --preset debug -L assignment
ctest --preset debug -L perf
```

Individual binaries:

```bash
./build/debug/infrastructure_tests
./build/debug/assignment_tests
./build/debug/disk_scheduler_perf_test
```

## Sanitizers

```bash
cmake --preset asan  && cmake --build --preset asan  && ctest --preset asan
cmake --preset ubsan && cmake --build --preset ubsan && ctest --preset ubsan
cmake --preset tsan  && cmake --build --preset tsan  && ctest --preset tsan
```

Run TSan after the single-threaded tests pass. TSan is slower but exposes the bug class this assignment is designed to surface.

## Libraries and Tools

| Name | Kind | Use |
|---|---|---|
| `fmt` | library | Type-safe string formatting in diagnostics and the demo program. |
| `CLI11` | library | Command-line parsing for the demo binary. Not used inside the buffer pool. |
| `Catch2` | library | Unit-test framework. Tests should be direct; do not wrap Catch2 in a custom framework. |
| `cmake`, `make` | tool | Build configuration and execution. |
| `clang-format` | tool | Mechanical formatting; rerun before committing. |
| `clang-tidy` | tool | Compiler-driven static analysis. |
| AddressSanitizer | tool | Out-of-bounds, use-after-free, double free. |
| UndefinedBehaviorSanitizer | tool | Invalid casts, signed overflow, misaligned access. |
| ThreadSanitizer | tool | Data races on `page_table_`, frame metadata, pin counts, dirty bits, replacer state. |
| `ccache` | tool | Optional; caches compilation results. |

The three libraries are fetched by CMake. Sanitizer passes are necessary evidence for a concurrent C++ data structure but not sufficient. The API contract and invariants in the assignment writeup must also hold.

## Assertion Macros

`include/common/assert.hpp` provides:

* `LADA_ASSERT(cond, msg)` — debug-only check, compiled out under `NDEBUG`.
* `LADA_ENSURE(cond, msg)` — always-on check; prints `file:line:msg` and aborts.

## Files to Edit

```
src/buffer/arc_replacer.cpp
src/storage/disk_scheduler.cpp
src/storage/page_guard.cpp
src/buffer/buffer_pool_manager.cpp
```

Public headers in `include/` define the contract and must remain ABI-compatible with the visible tests.
