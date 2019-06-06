

inline vec3::vec3() 
{
#ifdef _DEBUG
	const int nan = -1;
	z = y = x = *(const float*)&nan;
#endif
}

inline vec3::vec3( float v )
{
	z = y = x = v;
}

inline vec3::vec3( float x0, float y0, float z0 ) : 
	x(x0), y(y0), z(z0)
{
	assert( check(*this) );
}

inline vec3::vec3( const vec2& v0, float z0 ) : 
	x(v0.x), y(v0.y), z(z0)
{
	assert( check(*this) );
}

inline void vec3::set( float x0, float y0, float z0 )
{
	x=x0;
	y=y0;
	z=z0;
}

inline vec3& vec3::operator+=( const vec3& o )
{
	assert( check(o) );
	assert( check(*this) );
	x+=o.x;
	y+=o.y;
	z+=o.z;
	return *this;
}

inline vec3& vec3::operator-=( const vec3& o )
{
	assert( check(o) );
	assert( check(*this) );
	x-=o.x;
	y-=o.y;
	z-=o.z;
	return *this;
}

inline vec3& vec3::operator*=( float s )
{
	assert( check(s) );
	assert( check(*this) );
	x*=s;
	y*=s;
	z*=s;
	return *this;
}

inline vec3& vec3::operator*=( const vec3& o )
{
	assert( check(o) );
	assert( check(*this) );
	x *= o.x;
	y *= o.y;
	z *= o.z;
	return *this;
}

inline vec3& vec3::operator/=( const vec3& o )
{
	assert( check(o) );
	assert( check(*this) );
	assert( fabsf(o.x) > FLT_MIN );
	assert( fabsf(o.y) > FLT_MIN );
	assert( fabsf(o.z) > FLT_MIN );
	x /= o.x;
	y /= o.y;
	z /= o.z;
	return *this;
}

inline vec3& vec3::operator/=( float s )
{
	assert( check(*this) );
	assert( check(s) );
	assert( fabsf(s) >= FLT_MIN ); // s must be != 0
	const float sinv = 1.f/s;
	x *= sinv;
	y *= sinv;
	z *= sinv;
	return *this;
}

inline vec3 vec3::operator/( float s ) const
{
	assert( check(*this) );
	assert( check(s) );
	assert( fabsf(s) >= FLT_MIN ); // s must be != 0
	const float sinv = 1.f/s;
	return vec3( x*sinv, y*sinv, z*sinv );
}

inline float& vec3::operator[]( size_t i )
{
	assert( i < 3 );
	return (&x)[i];
}

inline vec2& vec3::xy()
{
	return *(vec2*)&x;
}

inline vec3 vec3::operator*( const vec3& o ) const
{
	assert( check(o) );
	assert( check(*this) );
	return vec3(x*o.x,y*o.y,z*o.z);
}

inline vec3 vec3::operator/( const vec3& o ) const
{
	assert( check(o) );
	assert( check(*this) );
	assert( fabsf(o.x) > FLT_MIN );
	assert( fabsf(o.y) > FLT_MIN );
	assert( fabsf(o.z) > FLT_MIN );
	return vec3(x/o.x,y/o.y,z/o.z);
}

inline vec3 vec3::operator+( const vec3& o ) const
{
	assert( check(o) );
	assert( check(*this) );
	return vec3(x+o.x,y+o.y,z+o.z);
}

inline vec3 vec3::operator-( const vec3& o ) const
{
	assert( check(o) );
	assert( check(*this) );
	return vec3(x-o.x,y-o.y,z-o.z);
}

inline vec3 vec3::operator-() const
{
	assert( check(*this) );
	return vec3(-x,-y,-z);
}

inline vec3 vec3::operator*( float s ) const
{
	assert( check(s) );
	assert( check(*this) );
	return vec3(x*s,y*s,z*s);
}

inline const float& vec3::operator[]( size_t i ) const
{
	assert( check(*this) );
	assert( i < 3 );
	return (&x)[i];
}

inline bool vec3::operator==( const vec3& o ) const
{
	assert( check(o) );
	assert( check(*this) );
	return x==o.x && y==o.y && z==o.z;
}

inline bool vec3::operator!=( const vec3& o ) const
{
	return !(*this == o);
}

inline const vec2& vec3::xy() const
{
	return *(const vec2*)&x;
}

inline vec3 operator*( float s, const vec3& v )
{
	assert( check(s) );
	assert( check(v) );
	return vec3(v.x*s,v.y*s,v.z*s);
}

inline float length( const vec3& v )
{
	assert( check(v) );

	float res = sqrtf( v.x*v.x + v.y*v.y + v.z*v.z );

	assert( res >= 0.f && res <= FLT_MAX );
	return res;
}

inline float dot( const vec3& a, const vec3& b )
{
	assert( check(a) );
	assert( check(b) );

	float res = a.x*b.x + a.y*b.y + a.z*b.z;

	assert( res >= -FLT_MAX && res <= FLT_MAX );
	return res;
}

inline vec3 max( const vec3& a, const vec3& b )
{
	return vec3( max(a[0],b[0]), max(a[1],b[1]), max(a[2],b[2]) );
}

inline vec3 min( const vec3& a, const vec3& b )
{
	return vec3( min(a[0],b[0]), min(a[1],b[1]), min(a[2],b[2]) );
}

inline vec3 abs( const vec3& v )
{
	return vec3( fabsf(v[0]), fabsf(v[1]), fabsf(v[2]) );
}

inline vec3 mix( const vec3& x, const vec3& y, float a )
{
	return vecMix( x, y, a );
}

inline float distance( const vec3& p0, const vec3& p1 )
{
	return length( p0 - p1 );
}

inline vec3 clamp( const vec3& v, const vec3& min, const vec3& max )
{
	return vecClamp( v, min, max );
}

inline vec3 saturate( const vec3& v )
{
	return vecSaturate( v );
}

inline bool check( const vec3& v )
{
	return vecCheck( v );
}

inline vec3 neg( const vec3& a )
{
	return -a;
}

inline vec3 add( const vec3& a, const vec3& b )
{
	return a+b;
}

inline vec3 sub( const vec3& a, const vec3& b )
{
	return a-b;
}

inline vec3 mul( const vec3& a, float b )
{
	return a*b;
}

inline vec3 mul( float b, const vec3& a )
{
	return a*b;
}

// This file is part of 'slmath' C++ library. Copyright (C) 2009 Jani Kajala (kajala@gmail.com). See http://sourceforge.net/projects/slmath/
