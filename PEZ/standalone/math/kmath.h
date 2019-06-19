/*
	kmath.h
*/

#pragma once

#ifndef _koala_math_h__
#define _koala_math_h__

#include "../koala.h"
#include <cmath>
#include <limits>
#include <algorithm>

namespace math
{
using namespace std;
using koala::float_t;

#if !defined(M_PI)
	#define M_E        2.71828182845904523536
	#define M_LOG2E    1.44269504088896340736
	#define M_LOG10E   0.434294481903251827651
	#define M_LN2      0.693147180559945309417
	#define M_LN10     2.30258509299404568402
	#define M_PI       3.14159265358979323846
	#define M_PI_2     1.57079632679489661923
	#define M_PI_4     0.785398163397448309616
	#define M_1_PI     0.318309886183790671538
	#define M_2_PI     0.636619772367581343076
	#define M_2_SQRTPI 1.12837916709551257390
	#define M_SQRT2    1.41421356237309504880
	#define M_SQRT1_2  0.707106781186547524401
#endif

#if defined(min)
	#undef min
	#undef max
#endif

template <class T> inline static float_t min( float_t a, T b ) { return (std::min)(a, float_t(b)); }
template <class T> inline static float_t max( float_t a, T b ) { return (std::max)(a, float_t(b)); }

template <class T> inline         int trunc( T x )           { return ( x >= 0 ) ? int(x) : -int(-x); }

template< typename T> inline static T square( T x )          { return x * x; }
template <class T, class Q> inline  T lerp( T a, T b, Q t )  { return T( a * (1 - t) + b * t ); }
template <class T> inline           T clamp( T a, T l, T h ) { return (a < l) ? l : ( (a > h) ? h : a ); }

template <class T> inline  T lerpReference( const T &a, const T &b, float t )  { return T( a * (1 - t) + b * t ); }

template <class T> inline           T lerpfactor( T m, T a, T b )
{
	// Return how far m is between a and b, that is return t such that
	// if t = lerpfactor(m, a, b);
	// then m = lerp(a, b, t);
	//
	// If a==b, return 0.

	const T d = b - a, n = m - a;
	if ( std::abs(d) > T(1) || std::abs(n) < (std::numeric_limits<T>::max()) * std::abs(d) )
		return n / d;

	return T(0);
}

template <class T> inline bool
equalWithAbsError( T x1, T x2, T e )
{
	return ((x1 > x2)? x1 - x2: x2 - x1) <= e;
}

template <class T> inline bool
equalWithRelError( T x1, T x2, T e )
{
	return ((x1 > x2)? x1 - x2 : x2 - x1) <= e * ((x1 > 0)? x1: -x1);
}

template <class T> inline bool
fEqual( T a, T b=0, T tolerance = std::numeric_limits<T>::epsilon() )
{
	return equalWithAbsError(a, b, tolerance);
	// return math::abs(b-a) <= tolerance ? true : false;
}

template< typename T> inline static T inverse( T x )          { return x ? T(1)/x : 0; }
template< typename T> inline static T inverseSqrt( T x )      { return T(1) / math::sqrt(x); }

const static  double kRadiansToDegrees = ( 180.0 / M_PI ), kDegreesToRadians = 0.0174532925;
template< typename T> inline static T radiansToDegrees( T radians ) { return radians * T(kRadiansToDegrees); }
template< typename T> inline static T degreesToRadians( T degrees ) { return degrees * T(kDegreesToRadians); }

}

/*
NAMESPACE_KOALA_

template < class T = float_t >
struct Math
{
	static T	acos  (T x)      { return ::acos (double(x)); }
	static T	asin  (T x)      { return ::asin (double(x)); }
	static T	atan  (T x)      { return ::atan (double(x)); }
	static T	atan2 (T x, T y) { return ::atan2 (double(x), double(y)); }
	static T	cos   (T x)      { return ::cos (double(x)); }
	static T	sin   (T x)      { return ::sin (double(x)); }
	static T	tan   (T x)      { return ::tan (double(x)); }
	static T	cosh  (T x)      { return ::cosh (double(x)); }
	static T	sinh  (T x)      { return ::sinh (double(x)); }
	static T	tanh  (T x)      { return ::tanh (double(x)); }
	static T	exp   (T x)      { return ::exp (double(x)); }
	static T	log   (T x)      { return ::log (double(x)); }
	static T	log10 (T x)      { return ::log10 (double(x)); }
	static T	modf  (T x, T *iptr)
	{
		double ival;
		T rval( ::modf (double(x),&ival));
		*iptr = ival;
		return rval;
	}
	static T	pow   (T x, T y)	{ return ::pow (double(x), double(y)); }
	static T	sqrt  (T x)       { return ::sqrt (double(x)); }
	static T	ceil  (T x)       { return ::ceil (double(x)); }
	static T	fabs  (T x)       { return ::fabs (double(x)); }
	static T	floor (T x)       { return ::floor (double(x)); }
	static T	fmod  (T x, T y)  { return ::fmod (double(x), double(y)); }
	static T	hypot (T x, T y)	{ return ::hypot (double(x), double(y)); }
	inline static T square(T x) { return x * x; }

};


template <>
struct Math<float>
{
	static float	acos  (float x)           { return ::acosf (x); }	
	static float	asin  (float x)           { return ::asinf (x); }
	static float	atan  (float x)           { return ::atanf (x); }
	static float	atan2 (float x, float y)	{ return ::atan2f (x, y); }
	static float	cos   (float x)           { return ::cosf (x); }
	static float	sin   (float x)           { return ::sinf (x); }
	static float	tan   (float x)           { return ::tanf (x); }
	static float	cosh  (float x)           { return ::coshf (x); }
	static float	sinh  (float x)           { return ::sinhf (x); }
	static float	tanh  (float x)           { return ::tanhf (x); }
	static float	exp   (float x)           { return ::expf (x); }
	static float	log   (float x)           { return ::logf (x); }
	static float	log10 (float x)           { return ::log10f (x); }
	static float	modf  (float x, float *y)	{ return ::modff (x, y); }
	static float	pow   (float x, float y)	{ return ::powf (x, y); }
	static float	sqrt  (float x)           { return ::sqrtf (x); }
	static float	ceil  (float x)           { return ::ceilf (x); }
	static float	fabs  (float x)           { return ::fabsf (x); }
	static float	floor (float x)           { return ::floorf (x); }
	static float	fmod  (float x, float y)	{ return ::fmodf (x, y); }

	#if !defined(_MSC_VER)
		static float	hypot (float x, float y)  { return ::hypotf (x, y); }
	#else
		static float hypot (float x, float y)   { return ::sqrtf(x*x + y*y); }
	#endif

	inline static float square(float x) { return x * x; }
};

_NAMESPACE_KOALA
*/

#endif  /* _koala_math_h__ */
