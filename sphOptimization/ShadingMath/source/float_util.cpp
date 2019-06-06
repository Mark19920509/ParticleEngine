#include <slmath/float_util.h>


SLMATH_BEGIN()

float cot( float z )
{
	float tanz = tanf(z);
	assert( tanz < -FLT_MIN || tanz > FLT_MIN );
	return 1.f / tanz;
}

float smoothstep( float edge0, float edge1, float v )
{
	assert( edge1-edge0 >= FLT_MIN );
	float t = clamp( (v - edge0)/(edge1 - edge0), 0.f, 1.f );
	return t*t*(3.f - 2.f*t);
}

void getGaussianBlurKernel1D( int s, float o, float* kernel, size_t kernelsize )
{
	assert( s >= 1 );
	assert( o >= 0.f );
	assert( kernelsize >= size_t(s) ); kernelsize=kernelsize;
	assert( kernel );

	if ( 0.f == o )
		o = ((float(s)-1.f)*.5f - 1.f)*.3f + .8f;
	const int r = (s-1)/2;
	float sum = 0.f;

	for ( int i = 0 ; i < s ; ++i )
	{
		const float d = fabsf( float(i-r) );
		const float f = 1.f/(2.506628274631000502415765284811f*o*o) * exp( -d*d/(2.f*o*o) );
		kernel[i] = f;
		sum += f;
	}

	assert( sum > FLT_MIN );
	if ( sum > FLT_MIN )
	{
		const float inv_sum = 1.f/sum;
		for ( size_t i = 0 ; i < size_t(s) ; ++i )
			kernel[i] *= inv_sum;
	}
}

void getGaussianBlurKernel2D( int s, float o, float* kernel, size_t kernelsize )
{
	assert( s >= 1 );
	assert( o >= 0.f );
	assert( kernelsize >= size_t(s*s) ); kernelsize=kernelsize;
	assert( kernel );

	if ( 0.f == o )
		o = ((float(s)-1.f)*.5f - 1.f)*.3f + .8f;
	const int r = (s-1)/2;
	float sum = 0.f;

	for ( int j = 0 ; j < s ; ++j )
	{
		const float dy = float(j-r);
		for ( int i = 0 ; i < s ; ++i )
		{
			const float dx = float(i-r);
			const float f = 1.f/(6.283185307179586476925286766559f*o*o) * exp( -(dx*dx + dy*dy)/(2.f*o*o) );
			kernel[i+j*s] = f;
			sum += f;
		}
	}

	assert( sum > FLT_MIN );
	if ( sum > FLT_MIN )
	{
		const float inv_sum = 1.f/sum;
		const size_t s2 = s*s;
		for ( size_t i = 0 ; i < s2 ; ++i )
			kernel[i] *= inv_sum;
	}
}

bool check( const float* v, size_t n )
{
	for ( size_t i = 0 ; i < n ; ++i )
		if ( !check(v[i]) )
			return false;
	return true;
}

SLMATH_END()

// This file is part of 'slmath' C++ library. Copyright (C) 2009 Jani Kajala (kajala@gmail.com). See http://sourceforge.net/projects/slmath/

