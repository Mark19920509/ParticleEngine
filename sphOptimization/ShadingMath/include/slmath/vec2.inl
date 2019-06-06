inline vec2::vec2() 
{
#ifdef _DEBUG
	const int nan = -1;
	y = x = *(const float*)&nan;
#endif
}

inline vec2::vec2( float v )
{
	y = x = v;
}

inline vec2::vec2( float x0, float y0 ) : 
	x(x0), y(y0)
{
	assert( check(*this) );
}

inline void vec2::set( float x0, float y0 )
{
	x=x0;
	y=y0;
}

inline vec2& vec2::operator+=( const vec2& o )
{
	assert( check(o) );
	assert( check(*this) );
	x += o.x;
	y += o.y;
	return *this;
}

inline vec2& vec2::operator-=( const vec2& o )
{
	assert( check(o) );
	assert( check(*this) );
	x -= o.x;
	y -= o.y;
	return *this;
}

inline vec2& vec2::operator*=( float s )
{
	assert( check(*this) );
	assert( check(s) );
	x *= s;
	y *= s;
	return *this;
}

inline vec2& vec2::operator/=( float s )
{
	assert( check(*this) );
	assert( check(s) );
	assert( fabsf(s) >= FLT_MIN ); // s must be != 0
	const float sinv = 1.f/s;
	x *= sinv;
	y *= sinv;
	return *this;
}

inline vec2 vec2::operator/( float s ) const
{
	assert( check(*this) );
	assert( check(s) );
	assert( fabsf(s) >= FLT_MIN ); // s must be != 0
	const float sinv = 1.f/s;
	return vec2( x*sinv, y*sinv );
}

inline vec2& vec2::operator*=( const vec2& o )
{
	assert( check(o) );
	assert( check(*this) );
	x *= o.x;
	y *= o.y;
	return *this;
}

inline vec2& vec2::operator/=( const vec2& o )
{
	assert( check(o) );
	assert( check(*this) );
	assert( fabsf(o.x) > FLT_MIN );
	assert( fabsf(o.y) > FLT_MIN );
	x /= o.x;
	y /= o.y;
	return *this;
}

inline float& vec2::operator[]( size_t i )
{
	assert( i < 2 );
	return (&x)[i];
}

inline vec2 vec2::operator*( const vec2& o ) const
{
	assert( check(o) );
	assert( check(*this) );
	return vec2(x*o.x,y*o.y);
}

inline vec2 vec2::operator/( const vec2& o ) const
{
	assert( check(o) );
	assert( check(*this) );
	assert( fabsf(o.x) > FLT_MIN );
	assert( fabsf(o.y) > FLT_MIN );
	return vec2(x/o.x,y/o.y);
}

inline vec2 vec2::operator+( const vec2& o ) const
{
	assert( check(o) );
	assert( check(*this) );
	return vec2(x+o.x,y+o.y);
}

inline vec2 vec2::operator-( const vec2& o ) const
{
	assert( check(o) );
	assert( check(*this) );
	return vec2(x-o.x,y-o.y);
}

inline vec2 vec2::operator-() const
{
	assert( check(*this) );
	return vec2(-x,-y);
}

inline vec2 vec2::operator*( float s ) const
{
	assert( check(s) );
	assert( check(*this) );
	return vec2(x*s,y*s);
}

inline const float& vec2::operator[]( size_t i ) const
{
	assert( check(*this) );
	assert( i < 2 );
	return (&x)[i];
}

inline bool vec2::operator==( const vec2& o ) const
{
	return x==o.x && y==o.y;
}

inline bool vec2::operator!=( const vec2& o ) const
{
	return !(*this == o);
}

inline float length( const vec2& v )
{
	assert( check(v) );

	float res = sqrtf( v.x*v.x + v.y*v.y );

	assert( res >= 0.f && res <= FLT_MAX );
	return res;
}

inline float dot( const vec2& a, const vec2& b )
{
	assert( check(a) );
	assert( check(b) );

	float res = a.x*b.x + a.y*b.y;

	assert( res >= -FLT_MAX && res <= FLT_MAX );
	return res;
}

inline vec2	operator*( float s, const vec2& v )		
{
	assert( check(s) );
	assert( check(v) ); 
	return vec2(v.x*s,v.y*s);
}

inline vec2 max( const vec2& a, const vec2& b )
{
	return vec2( max(a[0],b[0]), max(a[1],b[1]) );
}

inline vec2 min( const vec2& a, const vec2& b )
{
	return vec2( min(a[0],b[0]), min(a[1],b[1]) );
}

inline vec2 abs( const vec2& v )
{
	return vec2( fabsf(v[0]), fabsf(v[1]) );
}

inline vec2 mix( const vec2& x, const vec2& y, float a )
{
	return vecMix( x, y, a );
}

inline float distance( const vec2& p0, const vec2& p1 )
{
	return length( p0 - p1 );
}

inline vec2 clamp( const vec2& v, const vec2& min, const vec2& max )
{
	return vecClamp( v, min, max );
}

inline vec2 saturate( const vec2& v )
{
	return vecSaturate( v );
}

inline bool check( const vec2& v )
{
	return vecCheck( v );
}

inline vec2 neg( const vec2& a )
{
	return -a;
}

inline vec2 add( const vec2& a, const vec2& b )
{
	return a+b;
}

inline vec2 sub( const vec2& a, const vec2& b )
{
	return a-b;
}

inline vec2 mul( const vec2& a, float b )
{
	return a*b;
}

inline vec2 mul( float b, const vec2& a )
{
	return a*b;
}

// This file is part of 'slmath' C++ library. Copyright (C) 2009 Jani Kajala (kajala@gmail.com). See http://sourceforge.net/projects/slmath/
