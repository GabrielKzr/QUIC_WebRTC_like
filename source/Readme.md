(# QUIC_WebRTC_Like (refactoring)

This folder contains the refactored C code and CMake build for the QUIC/WebRTC-like experiments.

## Project layout

- `inc/` — public headers
- `src/` — library sources (core implementation)
- `test_examples/` — tests and example executables (each test provides its own `main`)
- `CMakeLists.txt` — top-level CMake that builds a static library `quic_webrtc_lib` and adds `test_examples`

## Goals

- Build common sources into a static library `quic_webrtc_lib`.
- Place `main`/entry points in test directories under `test_examples/` so you can build only the tests you want.
- Allow building a single target using CMake's `--target` option.

## Build (quick start)

From `refactoring/`:

```bash
mkdir -p build
cmake -S . -B build -DENABLE_OSSLQUIC=OFF
cmake --build build -- -j$(nproc)
```

To build a specific test target (example):

```bash
# configure first (if not configured yet)
cmake -S . -B build -DENABLE_OSSLQUIC=OFF

# build only the chownat test
cmake --build build --target test_chownat

# or build the OpenSSL QUIC test (requires OpenSSL and -DENABLE_OSSLQUIC=ON at configure time)
cmake --build build --target test_openssl_quic
```

Notes:
- If you enable OpenSSL via `-DENABLE_OSSLQUIC=ON` when running `cmake`, the OpenSSL sources and link flags are added to the library and the OpenSSL test target. Example:

```bash
cmake -S . -B build -DENABLE_OSSLQUIC=ON
cmake --build build --target test_openssl_quic
```

## Targets added by the CMake configuration

- `quic_webrtc_lib` — static library with core sources
- `test_chownat` — executable that compiles `test_chownat.c` and links with `quic_webrtc_lib`
- `test_openssl_quic` — executable for the OpenSSL test (links OpenSSL when enabled)

You can list available targets after configuration with:

```bash
cmake --build build --target help
```

or inspect the generated `build` directory (e.g., `build/compile_commands.json` if you enable it).

## IntelliSense / VSCode

Add or update `.vscode/settings.json` to point the C/C++ extension to the includes and compiler path. Example:

```json
{
	"cmake.sourceDirectory": "${workspaceFolder}/refactoring",
	"C_Cpp.intelliSenseEngine": "default",
	"C_Cpp.default.includePath": [
		"${workspaceFolder}/refactoring/inc"
	],
	"C_Cpp.default.cStandard": "c17",
	"C_Cpp.default.compilerPath": "/usr/bin/gcc"
}
```

If IntelliSense shows false errors, run the command palette: `C/C++: Reset IntelliSense Database`.

## Adding new tests

Each test should live under `test_examples/<subdir>` and provide its own `CMakeLists.txt` that declares an executable and lists its private/public dependencies. Example minimal `CMakeLists.txt` for a test:

```cmake
add_executable(my_test my_test.c)
target_link_libraries(my_test PRIVATE quic_webrtc_lib)
target_include_directories(my_test PRIVATE ${CMAKE_SOURCE_DIR}/inc)
target_compile_options(my_test PRIVATE -Wall -Wextra -std=c11)
```

Place that `CMakeLists.txt` inside `test_examples/<subdir>/` and update `test_examples/CMakeLists.txt` to add the subdirectory.

## Troubleshooting

- If CMake reports `does not contain a CMakeLists.txt`, create the missing `CMakeLists.txt` in that directory or update the top-level `CMakeLists.txt` to point to existing test subdirectories.
- If you get link errors like `undefined reference to main`, ensure the test executable includes or links an object that provides `main`, or the test itself defines `main`.

## Example: build only one target

```bash
cmake -S . -B build -DENABLE_OSSLQUIC=OFF
cmake --build build --target test_chownat
```

---
If you want, I can:
- run CMake configure here and list actual targets detected, or
- add more test subdirectories with example `CMakeLists.txt` files.

)
