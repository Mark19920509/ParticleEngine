#ifndef SLMATH_LUA_H
#define SLMATH_LUA_H

#include <slmath/slmath_pp.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>

/** 
 * Initializes slmath support to Lua state.
 * Only available if slmath was compiled with SLMATH_LUA_SUPPORT.
 * Example Lua script after luaopen_slmath(L) has been called:
 *
 * v = slmath.vec2(1,2);
 * assert( v.x == 1 );
 * assert( v.y == 2 );
 *
 * See slmath/samples/lua_test/runme.lua for more examples.
 * Note that operator overloading is not supported by Lua.
 *
 * @return 1 if ok, 0 if error.
 */
int luaopen_slmath( lua_State* L );

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SLMATH_LUA_H

// This file is part of 'slmath' C++ library. Copyright (C) 2009 Jani Kajala (kajala@gmail.com). See http://sourceforge.net/projects/slmath/
