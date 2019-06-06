#ifndef SLMATH_CONFIGURE_H
#define SLMATH_CONFIGURE_H

/** Enable SIMD extensions (if supported by this platform) */
#define SLMATH_SIMD

/** Enable namespace support, everything placed inside slmath namespace */
#define SLMATH_NAMESPACE

/** 
 * Enable Lua scripting support. 
 * Requires: lua.h found on INCLUDE path.
 * @see luaopen_slmath
 */
//#define SLMATH_LUA_SUPPORT

#endif // SLMATH_CONFIGURE_H

// This file is part of 'slmath' C++ library. Copyright (C) 2009 Jani Kajala (kajala@gmail.com). See http://sourceforge.net/projects/slmath/
