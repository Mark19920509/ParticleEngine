#ifndef SLMATH_VEC3_H
#define SLMATH_VEC3_H

#include <slmath/vec2.h>

namespace slmath
{
    class vec4;
}

SLMATH_BEGIN()

/**
 * 3-vector.
 *
 * Note naming convention: This class is starting with small letter since 
 * it is NOT initialized by the default constructor, much like int, float, etc. types.
 *
 * @ingroup vec_util
 */

class vec3
{
public:
	/** Constants related to the class. */
	enum Constants
	{
		/** Number of dimensions in this vector. */
		SIZE = 3,
	};

	/** The first component of the vector. */
	float x;

	/** The second component of the vector. */
	float y;

	/** The third component of the vector. */
	float z;

	/** 
	 * Constructs undefined 3-vector. 
	 */
	vec3();

	/** 
	 * Constructs vector with all components set to the same value.
	 * @param v Each of the vector components will be set to this value.
	 */
	explicit vec3( float v );

	/** 
	 * Constructs the vector from scalars. 
	 * @param x0 The vector X-component (0)
	 * @param y0 The vector Y-component (1)
	 * @param z0 The vector Z-component (2)
	 */
	vec3( float x0, float y0, float z0 );

    /** 
	 * Constructs the vector from vector 4.
     * Doesn't take w in account 
	 * @param v The vector 4
	 */
	vec3(const slmath::vec4 &v);

	/** 
	 * Constructs the vector from 2-vector and scalar. 
	 * @param v0 The vector XY-components (0-1)
	 * @param w0 The vector Z-component (2)
	 */
	vec3( const vec2& v0, float w0 );

	/** Sets vector value. */
	void		set( float x0, float y0, float z0 );

	/** Component wise addition. */
	vec3&		operator+=( const slmath::vec3& o );

	/** Component wise subtraction. */
	vec3&		operator-=( const slmath::vec3& o );

	/** Component wise scalar multiplication. */
	vec3&		operator*=( float s );

	/** Component wise scalar division. */
	vec3&		operator/=( float s );

	/** Component wise multiplication. */
	vec3&		operator*=( const slmath::vec3& o );

	/** Component wise division. */
	vec3&		operator/=( const slmath::vec3& o );

	/** Returns component at specified index. */
	float&		operator[]( size_t i );

	/** Returns sub-vector. */
	vec2&		xy();

	/** Component wise multiplication. */
	vec3		operator*( const slmath::vec3& o ) const;

	/** Component wise division. */
	vec3		operator/( const slmath::vec3& o ) const;

	/** Component wise addition. */
	vec3		operator+( const slmath::vec3& o ) const;

	/** Component wise subtraction. */
	vec3		operator-( const slmath::vec3& o ) const;

	/** Component wise subtraction. */
	vec3		operator-() const;

	/** Component wise scalar multiplication. */
	vec3		operator*( float s ) const;

	/** Component wise scalar division. */
	vec3		operator/( float s ) const;

	/** Returns component at specified index. */
	const float& operator[]( size_t i ) const;

	/** Returns true if vectors are bitwise equal. */
	bool		operator==( const slmath::vec3& o ) const;

	/** Returns true if vectors are bitwise inequal. */
	bool		operator!=( const slmath::vec3& o ) const;

	/** Returns sub-vector. */
	const vec2&	xy() const;
};


/** 
 * Returns cross product of two vectors. 
 * @ingroup vec_util
 */
slmath::vec3	cross( const slmath::vec3& a, const slmath::vec3& b );

/**
 * Calculates triangle face normal when triangle is defined by counter-clock-wise points.
 * @ingroup vec_util
 */
slmath::vec3	facenormal_ccw( const slmath::vec3& v0, const slmath::vec3& v1, const slmath::vec3& v2 );

/**
 * Calculates triangle face normal when triangle is defined by clock-wise points.
 * @ingroup vec_util
 */
slmath::vec3	facenormal_cw( const slmath::vec3& v0, const slmath::vec3& v1, const slmath::vec3& v2 );

/**
 * Refracts vector against specified normal using specified ratio of refraction indices.
 * The result is computed by
 * ndoti = dot(n,i);
 * k = 1.0f - eta*eta*(1.0f - ndoti*ndoti);
 * if k < 0 then return 0 else return eta*i-(eta*ndoti+sqrtf(k))*n
 * @param i Input vector. Must be pre-normalized.
 * @param n Normal vector. Must be pre-normalized.
 * @param eta Ratio of two materials refraction indices.
 * @return Refracted vector.
 * @ingroup vec_util
 */
slmath::vec3	refract( const slmath::vec3& i, const slmath::vec3& n, float eta );

/**
 * Reflects vector against specified normal.
 * @param i Input vector.
 * @param n Normal vector. Must be pre-normalized.
 * @return i - 2*dot(n,i)*n
 * @ingroup vec_util
 */
slmath::vec3	reflect( const slmath::vec3& i, const slmath::vec3& n );

/**
 * Checks if vector points to specified direction.
 * @param n Vector to be reflected if input does not face against reference vector.
 * @param i Input vector.
 * @param nref Normal reference vector.
 * @return If dot(nref<i) < 0 return n, otherwise return -n;
 * @ingroup vec_util
 */
slmath::vec3	faceforward( const slmath::vec3& n, const slmath::vec3& i, const slmath::vec3& nref );

/** 
 * Component wise scalar multiplication. 
 * @ingroup vec_util
 */
vec3			operator*( float s, const slmath::vec3& v );

/** 
 * Returns length of the vector. 
 * @ingroup vec_util
 */
float			length( const slmath::vec3& v );

/** 
 * Returns dot product (inner product) of two vectors. 
 * @ingroup vec_util
 */
float			dot( const slmath::vec3& a, const slmath::vec3& b );

/** 
 * Returns the vector normalized. 
 * @ingroup vec_util
 */
vec3			normalize( const slmath::vec3& v );

/**
 * Returns component wise maximum of two vectors.
 * @ingroup vec_util
 */
vec3			max( const slmath::vec3& a, const slmath::vec3& b );

/**
 * Returns component wise minimum of two vectors.
 * @ingroup vec_util
 */
vec3			min( const slmath::vec3& a, const slmath::vec3& b );

/**
 * Returns component wise absolute of the vector.
 * @ingroup vec_util
 */
vec3			abs( const slmath::vec3& v );

/** 
 * Returns linear blend between two values. Formula is x*(1-a)+y*a.
 * @param x The first value.
 * @param y The second value.
 * @param a The blend factor.
 * @return x*(1-a)+y*a
 * @ingroup vec_util
 */
slmath::vec3	mix( const slmath::vec3& x, const slmath::vec3& y, float a );

/** 
 * Returns distance between two values. 
 * @param p0 The first point vector.
 * @param p1 The second point vector.
 * @return Distance between p0 and p1, i.e. length(p1-p0).
 * @ingroup vec_util
 */
float			distance( const slmath::vec3& p0, const slmath::vec3& p1 );

/**
 * Returns value with components clamped between [min,max].
 * @ingroup vec_util
 */
slmath::vec3	clamp( const slmath::vec3& v, const slmath::vec3& min, const slmath::vec3& max );

/**
 * Returns value with components clamped between [0,1].
 * @ingroup vec_util
 */
slmath::vec3	saturate( const slmath::vec3& v );

/**
 * Returns true if all components are valid numbers.
 * @ingroup vec_util
 */
bool			check( const slmath::vec3& v );

/**
 * Returns negated vector. This function is useful for scripting where overloaded operators are not available.
 * @ingroup vec_util
 */
slmath::vec3	neg( const slmath::vec3& a );

/**
 * Adds two vectors together. This function is useful for scripting where overloaded operators are not available.
 * @ingroup vec_util
 */
slmath::vec3	add( const slmath::vec3& a, const slmath::vec3& b );

/**
 * Subtracts two vectors together. This function is useful for scripting where overloaded operators are not available.
 * @ingroup vec_util
 */
slmath::vec3	sub( const slmath::vec3& a, const slmath::vec3& b );

/**
 * Multiplies vector by scalar. This function is useful for scripting where overloaded operators are not available.
 * @ingroup vec_util
 */
slmath::vec3	mul( const slmath::vec3& a, float b );

/**
 * Multiplies vector by scalar. This function is useful for scripting where overloaded operators are not available.
 * @ingroup vec_util
 */
slmath::vec3	mul( float b, const slmath::vec3& a );

#include <slmath/vec3.inl>

SLMATH_END()

#endif // SLMATH_VEC3_H

// This file is part of 'slmath' C++ library. Copyright (C) 2009 Jani Kajala (kajala@gmail.com). See http://sourceforge.net/projects/slmath/
