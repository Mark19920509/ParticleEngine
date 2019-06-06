#include <slmath/vec2.h>

SLMATH_BEGIN()

vec2 normalize( const vec2& v )
{
	assert( check(v) );

	float len = length(v);
	assert( len >= FLT_MIN );
	
	float invlen = 1.f / len;
	vec2 res( v.x*invlen, v.y*invlen );
	assert( check(res) );
	return res;
}

SLMATH_END()

// This file is part of 'slmath' C++ library. Copyright (C) 2009 Jani Kajala (kajala@gmail.com). See http://sourceforge.net/projects/slmath/
