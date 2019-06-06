#ifndef SLMATH_PP_H
#define SLMATH_PP_H

#include <math.h>
#include <float.h>
#include <assert.h>
#include <slmath/slmath_configure.h>

// make sure <windows.h> does not define min and max as macros
#undef min
#undef max
#ifndef NOMINMAX
#define NOMINMAX
#endif

// Use this instead of namespaces directly to enable easier porting for platforms without namespace support
#ifdef SLMATH_NAMESPACE
	#define SLMATH_BEGIN() namespace slmath {
	#define SLMATH_END() }
	#define SLMATH_USING() using namespace slmath;
	#define SLMATH_NS(A) slmath::A
	namespace slmath {}
#else
	#define SLMATH_BEGIN()
	#define SLMATH_END()
	#define SLMATH_USING()
	#define SLMATH_NS(A) A
#endif

// Verify requested configuration for this build:
// SSE2 extensions supported for Visual Studio 2005 and newer
// (we *should* check CPU type at runtime also but we ignore it for now)
#if (_MSC_VER >= 1500)
	#if defined(SLMATH_SIMD) && (defined(_M_X64) || _M_IX86_FP >= 2)
		#define SLMATH_SSE2_MSVC
		#define SLMATH_MSVC_HAS_INTRIN_H
	#endif
#elif (_MSC_VER >= 1300)
	// Enable SSE2 in Visual Studio 2003
	// <intrin.h> is not available.
	#define SLMATH_SSE2_MSVC
#endif

// Auto-link library on MSVC
//#ifdef _MSC_VER
//	#ifdef _DEBUG
//		#pragma comment( lib, "slmath-Debug.lib" )
//	#else
//		#pragma comment( lib, "slmath-Release.lib" )
//	#endif
//#endif

// SIMD macros
#include <slmath/simd.h>

#endif

// This file is part of 'slmath' C++ library. Copyright (C) 2009 Jani Kajala (kajala@gmail.com). See http://sourceforge.net/projects/slmath/
