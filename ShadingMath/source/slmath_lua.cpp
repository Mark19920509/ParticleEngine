#include <slmath/slmath_pp.h>
#ifdef SLMATH_LUA_SUPPORT

#ifdef _MSC_VER
#pragma warning( disable : 4127 )
#endif

extern "C" {
#include <lua.h>
}

SLMATH_USING()

#define STATIC_LINKED
#include "../swig/slmath_wrap.cxx"

#endif // SLMATH_LUA_SUPPORT
