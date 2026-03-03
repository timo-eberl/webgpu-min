# Minimal WebGPU C Project

A barebones project to test WebGPU compilation from C to both WebAssembly (via Emscripten) and Native Linux (via GCC + `wgpu-native`).

## How It Works

The WebGPU API in C is defined by a standard header (`webgpu.h`), allowing the same source code to interface with different backend implementations depending on the compilation target.

*   **WebAssembly (Emscripten):** The `--use-port=emdawnwebgpu` compiler flag instructs Emscripten to provide the standard WebGPU C headers (maintained by the Dawn project) and link the corresponding JavaScript wrappers (from Dawn). At runtime, these wrappers translate the C API calls into the browser's native JavaScript WebGPU API. Dawn is Google's WebGPU implementation used in Chrome.
*   **Native Linux (GCC):** The C source is compiled against the headers provided by the `wgpu-native` [release](https://github.com/gfx-rs/wgpu-native/releases/), a Rust implementation of the WebGPU specification that maps the C API calls to native desktop graphics APIs like Vulkan. It's based on `wgpu`, which is used by Firefox.

## WebAssembly Build (Emscripten)

> More details here: https://dawn.googlesource.com/dawn/%2B/refs/heads/main/src/emdawnwebgpu/pkg/README.md

```bash
mkdir build
emcc main.c -o build/index.html --use-port=emdawnwebgpu
```

**To Run:** Start a local web server in the `build/` directory and open `http://localhost:8000` in a WebGPU-compatible browser:

```bash
python3 -m http.server --directory build
```

## Native Linux Build (GCC)

Compile using GCC. We statically link against `libwgpu_native.a` to create a portable executable.

```bash
mkdir build
gcc main.c -o build/app \
    -I./wgpu-native/include \
    ./wgpu-native/lib/libwgpu_native.a \
    -lm
```

**To Run:**
Execute the compiled binary directly from the build folder:
```bash
./build/app
```
