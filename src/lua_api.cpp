#include "lua_api.hpp"

#include <cstdio>
#include <string>

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

namespace {

#if defined(_WIN32)
using LibraryHandle = HMODULE;

LibraryHandle open_library(const char* path) {
    return LoadLibraryA(path);
}

void* resolve_symbol(LibraryHandle lib, const char* name) {
    return reinterpret_cast<void*>(GetProcAddress(lib, name));
}

std::string last_os_error() {
    const DWORD code = GetLastError();
    if (code == 0) {
        return "unknown error";
    }

    char* buffer = nullptr;
    const DWORD size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&buffer),
        0,
        nullptr);

    if (size == 0 || buffer == nullptr) {
        return "Win32 error " + std::to_string(code);
    }

    std::string message(buffer, size);
    LocalFree(buffer);

    while (!message.empty() && (message.back() == '\r' || message.back() == '\n' || message.back() == ' ')) {
        message.pop_back();
    }
    return message;
}
#else
using LibraryHandle = void*;

LibraryHandle open_library(const char* path) {
    return dlopen(path, RTLD_LAZY | RTLD_GLOBAL);
}

void* resolve_symbol(LibraryHandle lib, const char* name) {
    return dlsym(lib, name);
}

std::string last_os_error() {
    const char* err = dlerror();
    return err ? err : "unknown error";
}
#endif

const char* const* lua_shared_candidates() {
#if defined(GM_BYTECODE_TARGET_win64)
    static const char* const candidates[] = {
        "lua_shared.dll",
        "bin/win64/lua_shared.dll",
        "garrysmod/bin/win64/lua_shared.dll",
        "bin/lua_shared.dll",
        nullptr,
    };
#elif defined(GM_BYTECODE_TARGET_win32)
    static const char* const candidates[] = {
        "lua_shared.dll",
        "bin/lua_shared.dll",
        "garrysmod/bin/lua_shared.dll",
        nullptr,
    };
#elif defined(GM_BYTECODE_TARGET_linux64)
#  if defined(GM_BYTECODE_CLIENT)
    // Client prefers the non-_srv library.
    static const char* const candidates[] = {
        "lua_shared.so",
        "bin/linux64/lua_shared.so",
        "garrysmod/bin/linux64/lua_shared.so",
        "bin/lua_shared.so",
        "garrysmod/bin/lua_shared.so",
        "../lua_shared.so",
        "lua_shared_srv.so",
        "bin/linux64/lua_shared_srv.so",
        "garrysmod/bin/linux64/lua_shared_srv.so",
        nullptr,
    };
#  else
    static const char* const candidates[] = {
        "lua_shared_srv.so",
        "lua_shared.so",
        "bin/linux64/lua_shared_srv.so",
        "bin/linux64/lua_shared.so",
        "garrysmod/bin/linux64/lua_shared_srv.so",
        "garrysmod/bin/linux64/lua_shared.so",
        "bin/lua_shared_srv.so",
        "bin/lua_shared.so",
        "garrysmod/bin/lua_shared_srv.so",
        "garrysmod/bin/lua_shared.so",
        "../lua_shared_srv.so",
        "../lua_shared.so",
        nullptr,
    };
#  endif
#else
    // linux32 (GMod 32-bit branch)
#  if defined(GM_BYTECODE_CLIENT)
    static const char* const candidates[] = {
        "lua_shared.so",
        "bin/linux32/lua_shared.so",
        "garrysmod/bin/lua_shared.so",
        "bin/lua_shared.so",
        "../lua_shared.so",
        "lua_shared_srv.so",
        "bin/linux32/lua_shared_srv.so",
        "garrysmod/bin/lua_shared_srv.so",
        nullptr,
    };
#  else
    static const char* const candidates[] = {
        "lua_shared_srv.so",
        "lua_shared.so",
        "bin/linux32/lua_shared_srv.so",
        "bin/linux32/lua_shared.so",
        "garrysmod/bin/lua_shared_srv.so",
        "garrysmod/bin/lua_shared.so",
        "bin/lua_shared_srv.so",
        "bin/lua_shared.so",
        "../lua_shared_srv.so",
        "../lua_shared.so",
        nullptr,
    };
#  endif
#endif
    return candidates;
}

LibraryHandle find_lua_shared(std::string& error) {
    std::string attempts;

    for (const char* const* path = lua_shared_candidates(); *path != nullptr; ++path) {
        if (LibraryHandle lib = open_library(*path)) {
            return lib;
        }

        if (!attempts.empty()) {
            attempts += "; ";
        }
        attempts += *path;
        attempts += " (";
        attempts += last_os_error();
        attempts += ")";
    }

    error = "could not load lua_shared; tried: " + attempts;
    return nullptr;
}

template <typename T>
bool bind(LibraryHandle lib, const char* name, T& out, std::string& error) {
#if !defined(_WIN32)
    dlerror(); // clear any prior error before dlsym
#endif
    void* symbol = resolve_symbol(lib, name);
    if (!symbol) {
        error = std::string("missing required symbol '") + name + "' in lua_shared (" + last_os_error() + ")";
        return false;
    }
    out = reinterpret_cast<T>(symbol);
    return true;
}

} // namespace

bool LuaAPI::load() {
    if (loaded_) {
        return true;
    }

    error_.clear();

    LibraryHandle lib = find_lua_shared(error_);
    if (!lib) {
        std::fprintf(stderr, "bytecode: %s\n", error_.c_str());
        return false;
    }

    // Keep the library loaded for the lifetime of the process.
    const bool ok = bind(lib, "lua_settop", lua_settop, error_)
        && bind(lib, "lua_pushvalue", lua_pushvalue, error_)
        && bind(lib, "lua_pushlstring", lua_pushlstring, error_)
        && bind(lib, "lua_pushcclosure", lua_pushcclosure, error_)
        && bind(lib, "lua_settable", lua_settable, error_)
        && bind(lib, "lua_tolstring", lua_tolstring, error_)
        && bind(lib, "luaL_loadbuffer", luaL_loadbuffer, error_);

    if (!ok) {
        std::fprintf(stderr, "bytecode: %s\n", error_.c_str());
        return false;
    }

    loaded_ = true;
    return true;
}

LuaAPI& lua_api() {
    static LuaAPI api;
    static const bool initialized = api.load();
    (void)initialized;
    return api;
}
