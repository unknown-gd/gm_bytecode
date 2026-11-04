# gm_bytecode
It's a bytecode loader for gmod servers and clients

Repository: https://github.com/unknown-gd/gm_bytecode

## Usage
```lua
local bytecode = string.dump(function() print "my secrit code" end)
-- 10 minutes later
require("bytecode")
bytecode_load_bytecode(bytecode)()
```

Copy the matching modules into `garrysmod/lua/bin`:

| GMod branch | OS | Server | Client |
|---|---|---|---|
| x64 | Linux | `gmsv_bytecode_linux64.dll` | `gmcl_bytecode_linux64.dll` |
| x32 | Linux | `gmsv_bytecode_linux.dll` | `gmcl_bytecode_linux.dll` |
| x64 | Windows | `gmsv_bytecode_win64.dll` | `gmcl_bytecode_win64.dll` |
| x32 | Windows | `gmsv_bytecode_win32.dll` | `gmcl_bytecode_win32.dll` |

## Building

### Nix (flake) — Linux x64 / x32
Builds both `gmsv_` and `gmcl_` modules:

```sh
nix build                 # linux64 + linux32 → result/lib/
nix build .#linux64
nix build .#linux32
nix build .#all

nix develop               # cmake, ninja, gcc, i686 toolchain
```

### CMake presets
```sh
cmake --preset linux64 && cmake --build --preset linux64
cmake --preset linux32 && cmake --build --preset linux32

# Native MSVC on Windows
cmake --preset win64-msvc && cmake --build --preset win64-msvc
cmake --preset win32-msvc && cmake --build --preset win32-msvc
```

Or set the target explicitly:

```sh
cmake -B build -DGM_BYTECODE_TARGET=linux64|linux32|win64|win32 -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Manual (without presets)
```sh
git clone https://github.com/unknown-gd/gm_bytecode.git
cd gm_bytecode
```

**Linux**
```sh
cmake -B build-linux64 -DGM_BYTECODE_TARGET=linux64 -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux64

cmake -B build-linux32 -DGM_BYTECODE_TARGET=linux32 -DCMAKE_CXX_FLAGS=-m32 -DCMAKE_C_FLAGS=-m32 -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux32
```

**Windows (MSVC)**
```sh
cmake -B build-win64 -A x64 -DGM_BYTECODE_TARGET=win64 -DCMAKE_BUILD_TYPE=Release
cmake --build build-win64 --config Release

cmake -B build-win32 -A Win32 -DGM_BYTECODE_TARGET=win32 -DCMAKE_BUILD_TYPE=Release
cmake --build build-win32 --config Release
```
