#include "lua.hpp"

#include <cstdio>
#include <string>
#include <string_view>

#if defined(_WIN32)
#  define BYTECODE_EXPORT extern "C" __declspec(dllexport)
#else
#  define BYTECODE_EXPORT extern "C" __attribute__((visibility("default")))
#endif

namespace {

int push_error(LuaState& state, std::string_view message) {
    state.push_string(message);
    return 1;
}

int load_bytecode(lua_State* L) {
    LuaState state(L);

    if (!lua_api().ready()) {
        return push_error(state, "bytecode: Lua API is not initialized");
    }

    auto [code, len] = state.get_string_data(1);
    if (!code) {
        return push_error(
            state,
            "bytecode_load_bytecode: argument #1 must be a bytecode string");
    }

    if (len == 0) {
        return push_error(
            state,
            "bytecode_load_bytecode: argument #1 is empty");
    }

    if (state.load_buffer(code, len, "=bytecode") != 0) {
        auto [err, err_len] = state.get_string_data(-1);
        if (!err || err_len == 0) {
            state.pop();
            return push_error(
                state,
                "bytecode_load_bytecode: failed to load bytecode");
        }

        // Replace the raw compiler error with a prefixed message for clarity.
        std::string message = "bytecode_load_bytecode: ";
        message.append(err, err_len);
        state.pop();
        return push_error(state, message);
    }

    return 1;
}

} // namespace

BYTECODE_EXPORT int gmod13_open(lua_State* L) {
    if (!lua_api().ready()) {
        std::fprintf(
            stderr,
            "bytecode: refusing to register module (%s)\n",
            lua_api().error().empty() ? "Lua API is not initialized" : lua_api().error().c_str());
        return 0;
    }

    LuaState state(L);

    state.push_globals();
    state.push_string("bytecode_load_bytecode");
    state.push_function(load_bytecode);
    state.set_table(-3);
    state.pop();

    return 0;
}

BYTECODE_EXPORT int gmod13_close(lua_State* /*L*/) {
    return 0;
}
