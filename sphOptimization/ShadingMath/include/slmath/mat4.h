#ifndef SLMATH_MAT4_H
#define SLMATH_MAT4_H

#include <slmath/vec4.h>

SLMATH_BEGIN()

class quat;

/** 
 * 4x4 matrix. Columns vectors, column-major ordering.
 *
 * Note naming convention: This class is starting with small letter since 
 * it is NOT initialized by the default constructor, much like int, float, etc. types.
 *
 * To make combined transform of matrix W, then V and finally P:
 * WVP = P * V * W;
 *
 * To transform point with transform T:
 * v1 = T * v0;
 *
 * @ingroup mat_util
 */
#ifdef SWIG
class mat4
#else
SLMATH_ALIGN16 class mat4
#endif
{
public:
	/** Constructs undefined matrix. */
	mat4() {}

	/** 
	 * Constructs diagonal matrix. All diagonal elements specified value, rest 0. 
	 * @param d All diagonal elements are set to this value.
	 */
	explicit mat4( float d );

	/**
	 * Constructs rotation matrix from quaternion.
	 */
	explicit mat4( const slmath::quat& q );

	/**
	 * Constructs rotation matrix from angle-axis.
	 * @param a Rotation angle in radians.
	 * @param v Rotation axis.
	 */
	mat4( float a, const slmath::vec3& v );

	/** 
	 * Constructs matrix from 4 column vectors.
	 */
	mat4( const slmath::vec4& c0, const slmath::vec4& c1, const slmath::vec4& c2, const slmath::vec4& c3 );

	/** Sets ith column. */
	void			set( size_t i, const slmath::vec4& v );

	/** Returns specified column vector (0-based index). */
	slmath::vec4&	operator[]( size_t i );

	/** Component wise addition. */
	mat4&			operator+=( const mat4& o );

	/** Component wise subtraction. */
	mat4&			operator-=( const mat4& o );

	/** Component wise scalar multiplication. */
	mat4&			operator*=( float s );

	/** Matrix multiplication. */
	mat4&			operator*=( const mat4& o );

#ifndef SWIG
	/** 128-bit 4-vector storage access. */
	m128_t*			m128()		{return m_m128;}
#endif

	/** Returns ith column. */
	const slmath::vec4&		get( size_t i ) const;

	/** Component wise equality. */
	bool			operator==( const mat4& o ) const;

	/** Component wise inequality. */
	bool			operator!=( const mat4& o ) const;

	/** Component wise addition. */
	mat4			operator+( const mat4& o ) const;

	/** Component wise subtraction. */
	mat4			operator-( const mat4& o ) const;

	/** Component wise subtraction. */
	mat4			operator-() const;

	/** Component wise scalar multiplication. */
	mat4			operator*( float s ) const;

	/** Matrix multiplication. */
	mat4			operator*( const mat4& o ) const;

	/** Returns specified column vector (0-based index). */
	const slmath::vec4&		operator[]( size_t i ) const;

#ifndef SWIG
	/** 128-bit 4-vector storage access. */
	const m128_t*	m128() const	{return m_m128;}
#endif

private:
	/** Column vectors. */
	m128_t			m_m128[4];

	vec4*			v4()			{return reinterpret_cast<vec4*>(m_m128);}
	const vec4*		v4() const		{return reinterpret_cast<const vec4*>(m_m128);}
};

/** 
 * Transform column vector by matrix. 
 * @param v Column vector be multiplied by matrix.
 * @param m Matrix multiplying column vector.
 * @ingroup mat_util
 */
slmath::vec4	operator*( const mat4& m, const slmath::vec4& v );

/** 
 * Transform row vector by matrix. 
 * @param v Row vector be multiplied by matrix.
 * @param m Matrix multiplying column vector.
 * @ingroup mat_util
 */
slmath::vec4	operator*( const slmath::vec4& v, const mat4& m );

/** 
 * Transform column vector by matrix. 
 * Same as operator*, but for scripting languages not supporting operator overloading.
 * @param v Column vector be multiplied by matrix.
 * @param m Matrix multiplying column vector.
 * @ingroup mat_util
 */
slmath::vec4	mul( const slmath::mat4& m, const slmath::vec4& v );

/** 
 * Transform row vector by matrix. 
 * @param v Row vector be multiplied by matrix.
 * @param m Matrix multiplying column vector.
 * @ingroup mat_util
 */
slmath::vec4	mul( const slmath::vec4& v, const mat4& m );

/** 
 * Swaps column and row vectors with each other. 
 * @param m Matrix to be transposed.
 * @return Transposed matrix.
 * @ingroup mat_util
 */
slmath::mat4	transpose( const slmath::mat4& m );

/** 
 * Returns inverse of the matrix.
 * @param m Matrix to be inverted.
 * @return Inverted matrix.
 * @ingroup mat_util
 */
slmath::mat4	inverse( const slmath::mat4& m );

/**
 * Returns determinant of the matrix.
 * @ingroup mat_util
 */
float			det( const slmath::mat4& m );

/** 
 * Returns true if all components of the matrix are valid numbers. 
 * @ingroup mat_util
 */
bool			check( const slmath::mat4& v );

/** 
 * Returns right-handed perspective projection transform.
 * @param out [out] Receives the transform.
 * @param fovy Vertical field of view in radians.
 * @param aspect View space width divided by height.
 * @param znear Z-value at near view-plane. Distance to zfar must be more than 1e-6.
 * @param zfar Z-value at far view-plane. Distance to znear must be more than 1e-6.
 * @ingroup mat_util
 */
slmath::mat4	perspectiveFovRH( float fovy, float aspect, float znear, float zfar );

/** 
 * Returns left-handed perspective projection transform.
 * @param fovy Vertical field of view in radians.
 * @param aspect View space width divided by height.
 * @param znear Z-value at near view-plane. Distance to zfar must be more than 1e-6.
 * @param zfar Z-value at far view-plane. Distance to znear must be more than 1e-6.
 * @return The transform.
 * @ingroup mat_util
 */
slmath::mat4	perspectiveFovLH( float fovy, float aspect, float znear, float zfar );

/**
 * Returns righ-handed orthographic projection matrix.
 * @param w Width of view volume.
 * @param h Height of view volume.
 * @param znear Minimum Z-value of view volume.
 * @param zfar Maximum Z-value of view volume.
 */
slmath::mat4	orthoRH( float w, float h, float znear, float zfar );

/**
 * Returns left-handed orthographic projection matrix.
 * @param w Width of view volume.
 * @param h Height of view volume.
 * @param znear Minimum Z-value of view volume.
 * @param zfar Maximum Z-value of view volume.
 */
slmath::mat4	orthoLH( float w, float h, float znear, float zfar );

/**
 * Returns translation matrix.
 * @ingroup mat_util
 */
slmath::mat4	translation( const slmath::vec3& t );

/**
 * Returns rotation about X-axis.
 * @param a Rotation angle in radians.
 * @return The transform.
 * @ingroup mat_util
 */
slmath::mat4	rotationX( float a );

/**
 * Returns rotation about Y-axis.
 * @param a Rotation angle in radians.
 * @return The transform.
 * @ingroup mat_util
 */
slmath::mat4	rotationY( float a );

/**
 * Returns rotation about Z-axis.
 * @param a Rotation angle in radians.
 * @return The transform.
 * @ingroup mat_util
 */
slmath::mat4	rotationZ( float a );

/**
 * Returns uniform scaling matrix.
 * @param s Uniform scale factor.
 * @ingroup mat_util
 */
slmath::mat4	scaling( float s );

/**
 * Builds a right-handed inverse look-at (view-to-world) transformation matrix
 * targeting specified point along negative Z-axis.
 * @param eye Eye position.
 * @param at Look at target.
 * @param up World's up vector, e.g. (0,0,1) in 3dsmax.
 * @return The transform.
 * @ingroup mat_util
 */
slmath::mat4	targetAtRH( const slmath::vec3& eye, const slmath::vec3& at, const slmath::vec3& up );

/**
 * Builds a right-handed look-at (world-to-view) transformation matrix
 * targeting specified point along negative Z-axis.
 * @param eye Eye position.
 * @param at Look at target.
 * @param up World's up vector, e.g. (0,0,1) in 3dsmax.
 * @return The transform.
 * @ingroup mat_util
 */
slmath::mat4	lookAtRH( const slmath::vec3& eye, const slmath::vec3& at, const slmath::vec3& up );

/**
 * Builds a left-handed look-at (world-to-view) transformation matrix
 * targeting specified point along negative Z-axis.
 * @param eye Eye position.
 * @param at Look at target.
 * @param up World's up vector, e.g. (0,0,1) in 3dsmax.
 * @return The transform.
 * @ingroup mat_util
 */
slmath::mat4    lookAtLH( const vec3& eye, const vec3& at, const vec3& up );

/**
 * Creates right-handed world-to-cubemap-view transform for specified cube map face.
 * @param face Cube map face index. 0=X+, 1=X-, 2=Y+, 3=Y-, 4=Z+, 5=Z-. Defines view rotation.
 * @param worldpos Cube map center position in world space.
 * @return The world-to-cubemap-view transform.
 * @see cubeMapProjectionRH
 * @ingroup mat_util
 */
slmath::mat4	cubeMapViewRH( size_t face, const slmath::vec3& worldpos );

/**
 * Creates right-handed cubemap view-to-projection transform.
 * @param znear Near clip plane distance. Must be in range [1e-5f,1e6].
 * @param zfar Far clip plane distance. Must be in range (znear,1e6].
 * @return The view-to-projection transform.
 * @see cubeMapViewRH
 * @ingroup mat_util
 */
slmath::mat4	cubeMapProjectionRH( float znear, float zfar );

/**
 * Returns rotation matrix that rotates a vector to another vector.
 * @param from Direction to rotate from.
 * @param to Direction to rotate to. After the function 'from' vector points to this direction.
 * @return Rotation which rotates 'from' vector to 'to' vector.
 *
 * Based on code by Tomas Möller, John Hughes and article "Efficiently Building a 
 * Matrix to Rotate One Vector to Another", Journal of Graphics Tools, 4(4):1-4, 1999.
 * @ingroup mat_util
 */
slmath::mat4	fromToRotation( const slmath::vec3& from, const slmath::vec3& to );

/**
 * Builds frame of reference from normal vector.
 * Uses Gram-Schmidt orthonormalization.
 * @param normal Axis[2] of the frame. Must be unit vector.
 * @return Transformation frame which has input normal as axis[2].
 * @ingroup mat_util
 */
slmath::mat4	frameFromNormal( const slmath::vec3& normal );

/**
 * Returns outer product (tensor product) of two vectors.
 * @param a The first vector
 * @param b The second vector
 * @return Matrix resulting from outer product of vectors a and b.
 * @ingroup vec_util
 */
slmath::mat4	outerProduct( const slmath::vec4& a, const slmath::vec4& b );

#include <slmath/mat4.inl>

SLMATH_END()

#endif

// This file is part of 'slmath' C++ library. Copyright (C) 2009 Jani Kajala (kajala@gmail.com). See http://sourceforge.net/projects/slmath/
