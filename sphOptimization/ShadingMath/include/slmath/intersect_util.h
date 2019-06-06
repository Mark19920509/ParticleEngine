#ifndef SLMATH_INTERSECT_UTIL_H
#define SLMATH_INTERSECT_UTIL_H

#include <slmath/vec3.h>

SLMATH_BEGIN()

/**
 * \defgroup intersect_util Intersection testing helper functions.
 * @ingroup slmath
 */
/*@{*/

/*
 * Line segment parameters for the second (low-level) intersectLineBox function.
 * Use of this class is not needed but is provided as alternative if more extensive optimization is needed.
 * @see intersectLineBox
 */
class intersectLineBox_Line
{
public:
	/** Line segment start point. */
	const slmath::vec3	o;
	/** Vector from start point of the line segment to the end point. */
	const slmath::vec3	d;
	/** Component-wise one over delta (1/delta.x,1/delta.y,1/delta.z). */
	const slmath::vec3	inv_d;
	/** Signs (+-) of the line segment direction per component. */
	const int			signx;
	/** Signs (+-) of the line segment direction per component. */
	const int			signy;
	/** Signs (+-) of the line segment direction per component. */
	const int			signz;

	/** Sets ray origin and direction. Calculates intermediate values. */
	intersectLineBox_Line( const slmath::vec3& origin, const slmath::vec3& direction );

private:
	intersectLineBox_Line();
	intersectLineBox_Line( const intersectLineBox_Line& );
	intersectLineBox_Line& operator=( const intersectLineBox_Line& );
};

/**
 * Finds intersection between line segment and triangle.
 *
 * See: 'Fast, Minimum Storage Ray-Triangle Intersection' algorithm, 
 * Tomas Möller & Ben Trumbore, Volume 2, Number 1. Journal of Graphics Tools.
 *
 * @param o Starting point of the line segment.
 * @param d Vector from starting point of the line segment to the end point.
 * @param v0 Vertex 0 of the triangle.
 * @param v1 Vertex 1 of the triangle.
 * @param v2 Vertex 2 of the triangle.
 * @param t [out] Relative distance (0,1) to intersection.
 * @return true if intersect.
 */
bool	intersectLineTri( const slmath::vec3& o, const slmath::vec3& d, const slmath::vec3& v0, const slmath::vec3& v1, const slmath::vec3& v2, float* t );

/**
 * Finds if line segment and box intersect.
 *
 * See: Amy Williams, Steve Barrus, R. Keith Morley, and Peter Shirley
 * "An Efficient and Robust Ray-Box Intersection Algorithm"
 * Journal of graphics tools, 10(1):49-54, 2005.
 *
 * @param o Starting point of the line segment.
 * @param d Vector from starting point of the line segment to the end point.
 * @param boxmin Minimum coordinates of the box.
 * @param boxmax Maximum coordinates of the box.
 * @return true if intersect.
 */
bool	intersectLineBox( const slmath::vec3& o, const slmath::vec3& d, const slmath::vec3& boxmin, const slmath::vec3& boxmax );

/*
 * Finds if line segment and box intersect.
 * Uses pre-calculated information in intersectRayBox_Ray so a bit faster
 * than direct call version of intersectLineBox.
 *
 * See: Amy Williams, Steve Barrus, R. Keith Morley, and Peter Shirley
 * "An Efficient and Robust Ray-Box Intersection Algorithm"
 * Journal of graphics tools, 10(1):49-54, 2005.
 *
 * @param line Line segment information.
 * @param boxminmax Minimum and maximum coordinates (so array [2] of vec3) of the box.
 * @return true if intersect.
 */
//bool	intersectLineBox( const intersectLineBox_Line* line, const vec3* boxminmax );

/*@}*/

SLMATH_END()

#endif 

// This file is part of 'slmath' C++ library. Copyright (C) 2009 Jani Kajala (kajala@gmail.com). See http://sourceforge.net/projects/slmath/
