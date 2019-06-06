#ifndef SLMATH_VEC2_H
#define SLMATH_VEC2_H

#include <slmath/vec_impl.h>

SLMATH_BEGIN()

/**
 * 2-vector.
 *
 * Note naming convention: This class is starting with small letter since 
 * it is NOT initialized by the default constructor, much like int, float, etc. types.
 *
 * @ingroup vec_util
 */
class vec2
{
public:
	/** Constants related to the class. */
	enum Constants
	{
		/** Number of dimensions in this vector. */
		SIZE = 2,
	};

	/** The first component of the vector. */
	float x;

	/** The second component of the vector. */
	float y;

	/** 
	 * Constructs undefined 2-vector. 
	 */
	vec2();

	/** 
	 * Constructs vector with all components set to the same value.
	 * @param v Each of the vector components will be set to this value.
	 */
	explicit vec2( float v );

	/** 
	 * Constructs the vector from scalars. 
	 * @param x0 The vector X-component (0)
	 * @param y0 The vector Y-component (1)
	 */
	vec2( float x0, float y0 );

	/** Sets vector value. */
	void		set( float x0, float y0 );

	/** Component wise addition. */
	vec2&		operator+=( const slmath::vec2& o );

	/** Component wise subtraction. */
	vec2&		operator-=( const slmath::vec2& o );

	/** Component wise multiplication. */
	vec2&		operator*=( const slmath::vec2& o );

	/** Component wise division. */
	vec2&		operator/=( const slmath::vec2& o );

	/** Component wise scalar multiplication. */
	vec2&		operator*=( float s );

	/** Component wise scalar division. */
	vec2&		operator/=( float s );

	/** Returns component at specified index. */
	float&		operator[]( size_t i );

	/** Component wise multiplication. */
	vec2		operator*( const slmath::vec2& o ) const;

	/** Component wise division. */
	vec2		operator/( const slmath::vec2& o ) const;

	/** Component wise addition. */
	vec2		operator+( const slmath::vec2& o ) const;

	/** Component wise subtraction. */
	vec2		operator-( const slmath::vec2& o ) const;

	/** Component wise subtraction. */
	vec2		operator-() const;

	/** Component wise scalar multiplication. */
	vec2		operator*( float s ) const;

	/** Component wise scalar division. */
	vec2		operator/( float s ) const;

	/** Returns component at specified index. */
	const float& operator[]( size_t i ) const;

	/** Returns true if vectors are bitwise equal. */
	bool		operator==( const slmath::vec2& o ) const;

	/** Returns true if vectors are bitwise inequal. */
	bool		operator!=( const slmath::vec2& o ) const;
};


/** 
 * Component wise scalar multiplication. 
 * @ingroup vec_util
 */
slmath::vec2	operator*( float s, const slmath::vec2& v );

/** 
 * Returns length of the vector. 
 * @ingroup vec_util
 */
float			length( const slmath::vec2& v );

/** 
 * Returns dot product (inner product) of two vectors. 
 * @param a The first vector
 * @param b The second vector
 * @return a dot b
 * @ingroup vec_util
 */
float			dot( const slmath::vec2& a, const slmath::vec2& b );

/** 
 * Returns the vector normalized to unit (1) length.
 * @ingroup vec_util
 */
slmath::vec2	normalize( const slmath::vec2& v );

/** 
 * Returns true if all components of the vector are valid numbers. 
 * @ingroup vec_util
 */
bool			check( const slmath::vec2& v );

/**
 * Returns component wise maximum of two vectors.
 * @ingroup vec_util
 */
slmath::vec2	max( const slmath::vec2& a, const slmath::vec2& b );

/**
 * Returns component wise minimum of two vectors.
 * @ingroup vec_util
 */
slmath::vec2	min( const slmath::vec2& a, const slmath::vec2& b );

/**
 * Returns component wise absolute of the vector.
 * @ingroup vec_util
 */
slmath::vec2	abs( const slmath::vec2& v );

/** 
 * Returns linear blend between two values. Formula is x*(1-a)+y*a.
 * @param x The first value.
 * @param y The second value.
 * @param a The blend factor.
 * @return x*(1-a)+y*a
 * @ingroup vec_util
 */
slmath::vec2	mix( const slmath::vec2& x, const slmath::vec2& y, float a );

/** 
 * Returns distance between two values. 
 * @param p0 The first point vector.
 * @param p1 The second point vector.
 * @return Distance between p0 and p1, i.e. length(p1-p0).
 * @ingroup vec_util
 */
float			distance( const slmath::vec2& p0, const slmath::vec2& p1 );

/**
 * Returns value with components clamped between [min,max].
 * @ingroup vec_util
 */
slmath::vec2	clamp( const slmath::vec2& v, const slmath::vec2& min, const slmath::vec2& max );

/**
 * Returns value with components clamped between [0,1].
 * @ingroup vec_util
 */
slmath::vec2	saturate( const slmath::vec2& v );

/**
 * Returns true if all components are valid numbers.
 * @ingroup vec_util
 */
bool			check( const slmath::vec2& v );

/**
 * Returns negated vector. This function is useful for scripting where overloaded operators are not available.
 * @ingroup vec_util
 */
slmath::vec2	neg( const slmath::vec2& a );

/**
 * Adds two vectors together. This function is useful for scripting where overloaded operators are not available.
 * @ingroup vec_util
 */
slmath::vec2	add( const slmath::vec2& a, const slmath::vec2& b );

/**
 * Subtracts two vectors together. This function is useful for scripting where overloaded operators are not available.
 * @ingroup vec_util
 */
slmath::vec2	sub( const slmath::vec2& a, const slmath::vec2& b );

/**
 * Multiplies vector by scalar. This function is useful for scripting where overloaded operators are not available.
 * @ingroup vec_util
 */
slmath::vec2	mul( const slmath::vec2& a, float b );

/**
 * Multiplies vector by scalar. This function is useful for scripting where overloaded operators are not available.
 * @ingroup vec_util
 */
slmath::vec2	mul( float b, const slmath::vec2& a );

#include <slmath/vec2.inl>

SLMATH_END()

#endif

// This file is part of 'slmath' C++ library. Copyright (C) 2009 Jani Kajala (kajala@gmail.com). See http://sourceforge.net/projects/slmath/
