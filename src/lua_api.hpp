#pragma once

#include <cstddef>
#include <string>

struct lua_State;

using lua_CFunction = int (*)(lua_State *L);

struct LuaAPI {
    void (*lua_settop)(lua_State *L, int index) = nullptr;
    void (*lua_pushvalue)(lua_State *L, int index) = nullptr;
    void (*lua_pushlstring)(lua_State *L, const char *s, std::size_t len) = nullptr;
    void (*lua_pushcclosure)(lua_State *L, lua_CFunction fn, int n) = nullptr;
    void (*lua_settable)(lua_State *L, int index) = nullptr;
    const char *(*lua_tolstring)(lua_State *L, int index, std::size_t *len) = nullptr;
    int (*luaL_loadbuffer)(lua_State *L,
                           const char *buff,
                           std::size_t sz,
                           const char *name) = nullptr;
    int (*lua_error)(lua_State *L) = nullptr;

    bool ready() const { return loaded_; }
    const std::string &error() const { return error_; }

    bool load();

private:
    bool loaded_ = false;
    std::string error_;
};

LuaAPI &lua_api();
