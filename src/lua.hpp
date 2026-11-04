#pragma once

#include "lua_api.hpp"

#include <cstddef>
#include <string_view>
#include <utility>

inline constexpr int LUA_GLOBALSINDEX = -10002;

struct LuaState {
    lua_State *L = nullptr;

    explicit LuaState(lua_State *state) : L(state) {}

    void pop(int count = 1) const { set_top(-count - 1); }

    void set_top(int index) const { lua_api().lua_settop(L, index); }

    void push_value(int index) const { lua_api().lua_pushvalue(L, index); }

    void push_globals() const { push_value(LUA_GLOBALSINDEX); }

    void push_lstring(const char *data, std::size_t len) const {
        lua_api().lua_pushlstring(L, data, len);
    }

    void push_string(std::string_view data) const { push_lstring(data.data(), data.size()); }

    void push_function(lua_CFunction func) const { lua_api().lua_pushcclosure(L, func, 0); }

    void set_table(int index) const { lua_api().lua_settable(L, index); }

    // Returns {pointer, length}. pointer is null if the value is not a string.
    std::pair<const char *, std::size_t> get_string_data(int index) const {
        std::size_t len = 0;
        const char *ptr = lua_api().lua_tolstring(L, index, &len);
        return {ptr, len};
    }

    int load_buffer(const char *code, std::size_t len, const char *name) const {
        return lua_api().luaL_loadbuffer(L, code, len, name);
    }

    // Raises a Lua error from the value currently on top of the stack.
    // Uses longjmp — destroy C++ locals with non-trivial destructors first.
    [[noreturn]] void raise() const {
        lua_api().lua_error(L);
#if defined(_MSC_VER)
        __assume(0);
#else
        __builtin_unreachable();
#endif
    }

    // Pushes message, then raises. Safe with temporary std::string_view only.
    [[noreturn]] void error(std::string_view message) const {
        push_string(message);
        raise();
    }
};
