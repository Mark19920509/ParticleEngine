#include <slmath/vec4.h>

SLMATH_BEGIN()

vec4 normalize( const vec4& v )
{
	assert( check(v) );

	float len = length(v);
	assert( len >= FLT_MIN );
	
	float invlen = 1.f / len;
	vec4 res( v.x*invlen, v.y*invlen, v.z*invlen, v.w*invlen );
	assert( check(res) );
	return res;
}

float safeNormalize( const slmath::vec4& v, slmath::vec4& normal )
{
	assert( check(v) );

	float sqrLen = dot(v, v);
	float len;
    if (sqrLen != 0.0f)
    {
        len = sqrt(sqrLen);
        assert( sqrLen >= FLT_MIN );

	    float invlen = 1.f / len;
	    normal = vec4( v.x*invlen, v.y*invlen, v.z*invlen, v.w*invlen );
	    assert( check(normal) );
    }
    else
    {
        len = 0.0f;
        normal = vec4(0.0f);
    }
	return len;
}

SLMATH_END()

// This file is part of 'slmath' C++ library. Copyright (C) 2009 Jani Kajala (kajala@gmail.com). See http://sourceforge.net/projects/slmath/
