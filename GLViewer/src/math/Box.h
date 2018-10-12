/*
	Box.h
*/

#pragma once

#ifndef _koala_Box_h__
#define _koala_Box_h__

#include "../koala.h"
#include "Vector2.h"
#include "Vector3.h"
#include <limits>

NAMESPACE_KOALA_

struct Matrix4;

template <class T>
struct Boxt
{
	T min, max;

	static typename T::value_type baseTypeMin() { return -std::numeric_limits<typename T::value_type>::max(); }
	static typename T::value_type baseTypeMax() { return  std::numeric_limits<typename T::value_type>::max(); }

	Boxt() { makeEmpty(); }
	Boxt( const T &mn, const T &mx ) : min(mn), max(mx) {}

	void transform( const Matrix4 &m );
	void affine( const Matrix4 &m );

	inline T center() const { return ( max + min ) / 2; }
	inline T size() const { return isEmpty() ? T(0) : max - min; }

	void makeEmpty()
	{
			min = T(baseTypeMax());
			max = T(baseTypeMin());
	}

	void makeInfinite()
	{
			min = T(baseTypeMin());
			max = T(baseTypeMax());
	}

	inline bool isEmpty() const
	{
		for ( unsigned int i = 0; i < T::kNumElements; ++i )
		{
			if ( max[i] < min[i] )
				return true;
		}
		return false;
	}

	inline bool isInfinite() const
	{
		for ( unsigned int i = 0; i < T::kNumElements; ++i )
		{
			if ( min[i] != baseTypeMin() || max[i] != baseTypeMax() )
				return false;
		}

		return true;
	}

	bool intersects(const T &point) const
	{
		for ( unsigned int i = 0; i < T::kNumElements; ++i )
		{
			if ( point[i] < min[i] || point[i] > max[i] )
				return false;
		}

		return true;
	}

	void extendBy (const T &point )
	{
		for ( unsigned int i = 0; i < T::kNumElements; ++i )
		{
			if ( point[i] < min[i] )
				min[i] = point[i];

			if ( point[i] > max[i] )
				max[i] = point[i];
		}
	}

	void extendBy ( const Boxt<T> &box )
	{
		for ( unsigned int i = 0; i < T::kNumElements; ++i )
		{
			if (box.min[i] < min[i])
				min[i] = box.min[i];

			if (box.max[i] > max[i])
				max[i] = box.max[i];
		}
	}

};

typedef Boxt<Vector2i>  Box2i;
typedef Boxt<Vector2f>  Box2f;
typedef Boxt<Vector3>   Box3f;
typedef Box3f           Box3;
	
_NAMESPACE_KOALA

#endif /* _koala_Box_h__ */
