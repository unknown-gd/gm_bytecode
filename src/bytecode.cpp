#include "lua.hpp"

#include <cstdio>
#include <string>
#include <string_view>
#include <utility>

#if defined(_WIN32)
#define BYTECODE_EXPORT extern "C" __declspec(dllexport)
#else
#define BYTECODE_EXPORT extern "C" __attribute__((visibility("default")))
#endif

namespace {

bool is_bytecode_string(const char *data, std::size_t len) {
    if (data == nullptr || len < 3) {
        return false;
    }

    const auto *bytes = reinterpret_cast<const unsigned char *>(data);
    if (bytes[0] != 0x1b) {
        return false;
    }

    // PUC-Rio Lua dump: "\x1bLua"
    if (len >= 4 && bytes[1] == 'L' && bytes[2] == 'u' && bytes[3] == 'a') {
        return true;
    }

    // LuaJIT dump: "\x1bLJ"
    if (bytes[1] == 'L' && bytes[2] == 'J') {
        return true;
    }

    return false;
}

[[noreturn]] void raise_message(LuaState &state, std::string message) {
    state.push_string(message);
    // Free any heap buffer before lua_error longjmps past the destructor.
    std::string {}.swap(message);
    state.raise();
}

int load_bytecode(lua_State *L) {
    LuaState state(L);

    if (!lua_api().ready()) {
        state.error("bytecode: Lua API is not initialized");
    }

    auto [code, len] = state.get_string_data(1);
    if (!code) {
        state.error("load_bytecode: argument #1 must be a bytecode string");
    }

    if (len == 0) {
        state.error("load_bytecode: argument #1 is empty");
    }

    if (!is_bytecode_string(code, len)) {
        state.error("load_bytecode: argument #1 is not Lua/LuaJIT bytecode "
                    "(expected string.dump output)");
    }

    if (state.load_buffer(code, len, "=bytecode") != 0) {
        auto [err, err_len] = state.get_string_data(-1);
        if (!err || err_len == 0) {
            state.pop();
            state.error("load_bytecode: failed to load bytecode");
        }

        std::string message = "load_bytecode: ";
        message.append(err, err_len);
        state.pop();
        raise_message(state, std::move(message));
    }

    return 1;
}

} // namespace

BYTECODE_EXPORT int gmod13_open(lua_State *L) {
    LuaAPI &api = lua_api();

    if (!api.load()) {
        const char *detail =
            api.error().empty() ? "Lua API is not initialized" : api.error().c_str();
        std::fprintf(stderr, "bytecode: refusing to register module (%s)\n", detail);

        if (api.can_raise()) {
            LuaState state(L);
            std::string message = "bytecode: failed to initialize (";
            message += detail;
            message += ")";
            raise_message(state, std::move(message));
        }

        return 0;
    }

    LuaState state(L);

    state.push_globals();
    state.push_string("load_bytecode");
    state.push_function(load_bytecode);
    state.set_table(-3);
    state.pop();

    return 0;
}

BYTECODE_EXPORT int gmod13_close(lua_State * /*L*/) {
    lua_api().unload();
    return 0;
}
