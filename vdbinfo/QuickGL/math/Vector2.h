/*
	Vector2.h
*/

#pragma once

#ifndef _koala_Vector2_h__
#define _koala_Vector2_h__

#include "../koala.h"
#include "kmath.h"

NAMESPACE_KOALA_

template <class T>
struct Vector2t
{
	T x, y;

	enum { kNumElements = 2 };
	typedef T value_type;

	Vector2t() {}
	Vector2t( T a ) : x(a), y(a) {}
	Vector2t( T a, T b ) : x(a), y(b) {}
	template <class S> Vector2t( const Vector2t<S> &v ) : x(v.x), y(v.y) {}

	void set( T a, T b ) { x = a; y = b; }
	T&       operator[] (int i)       { return *((&x) + i); }
	const T& operator[] (int i) const { return *((&x) + i); }
	operator const T* () const { return &x; }

	Vector2t<T>  operator *  ( T f ) const { return Vector2t<T>(x * f, y * f); }
	Vector2t<T>& operator *= ( T f ) { x *= f; y *= f; return *this; }
	Vector2t<T>  operator *  ( const Vector2t<T> &v ) const { return Vector2t<T>(x * v.x, y * v.y); }
	Vector2t<T>& operator *= ( const Vector2t<T> &v ) { x *= v.x; y *= v.y; return *this; }
	Vector2t<T>  operator /  ( T f ) const { return Vector2t<T>(x / f, y / f); }
	Vector2t<T>& operator /= ( T f ) { x /= f; y /= f; return *this; }
	Vector2t<T>  operator /  ( const Vector2t<T> &v ) const { return Vector2t<T>(x / v.x, y / v.y); }
	Vector2t<T>& operator /= ( const Vector2t<T> &v ) { x /= v.x; y /= v.y; return *this; }
	Vector2t<T>  operator +  ( T f ) const { return Vector2t<T>(x + f, y + f); }
	Vector2t<T>& operator += ( T f ) { x += f; y += f; return *this; }
	Vector2t<T>  operator +  ( const Vector2t<T> &v ) const { return Vector2t<T>(x + v.x, y + v.y); }
	Vector2t<T>& operator += ( const Vector2t<T> &v ) { x += v.x; y += v.y; return *this; }
	Vector2t<T>  operator -  ( T f ) const { return Vector2t<T>(x - f, y - f); }
	Vector2t<T>& operator -= ( T f ) { x -= f; y -= f; return *this; }
	Vector2t<T>  operator -  ( const Vector2t<T> &v ) const { return Vector2t<T>(x - v.x, y - v.y); }
	Vector2t<T>& operator -= ( const Vector2t<T> &v ) { x -= v.x; y -= v.y; return *this; }

	bool     operator == ( const Vector2t<T> &v ) const { return x==v.x && y==v.y; }
	bool     operator != ( const Vector2t<T> &v ) const { return x!=v.x || y!=v.y; }

	// Negate
	Vector2t<T>  operator - () const { return Vector2t<T>(-x, -y); }

	// Also known as the absolute value or magnitude of the vector
	T length() const { return math::sqrt(x * x + y * y); }

	// Same as this dot this, length() squared
	T lengthSquared() const { return x * x + y * y; }

	// Same as (this-v).length()
	T distanceBetween( const Vector2t<T> &v ) const
	{
		return math::sqrt((x - v.x) * (x - v.x) + (y - v.y) * (y - v.y));
	}

	// Same as (this-v).lengthSquared()
	T distanceSquared( const Vector2t<T> &v ) const
	{
		return (x - v.x) * (x - v.x) + (y - v.y) * (y - v.y);
	}

	// Dot product. Twice the area of the triangle between the vectors.
	T dot( const Vector2t<T> &v ) const { return x * v.x + y * v.y; }

	// Cross product. This is a vector at right angles to the vectors with a length equal to the product of the lengths.
	T cross( const Vector2t<T> &v ) const
	{
		return x * v.y - y * v.x;
	}

	// Change the vector to be unit length. Returns the original length.
	T normalize()
	{
		const T d = length();
		if ( d )
			*this *= (float_t(1.0)/d);
		return d;
	}

	Vector2t<T> normalized() const
	{
		const T d = length();
		return d ? *this * (float_t(1.0)/d) : *this;
	}

	T sum()     const { return x+y; }
	Vector2t<T> inverse() const { return Vector2t<T>(x?1.0/x:0, y?1.0/y:0); }
	Vector2t<T> squared() const { return Vector2t<T>(math::square(x), math::square(y)); }

	float_t aspectRatio() const { return float_t(x)/float_t(y); }
	bool allAre ( const T v ) const { return x==v && y==v; }
	static Vector2t<T> zero() { return Vector2t<T>(0,0); }

};

typedef Vector2t<float_t> Vector2f;
typedef Vector2t<int>     Vector2i;
typedef Vector2f          Vector2;


_NAMESPACE_KOALA

#endif /* _koala_Vector2_h__ */
