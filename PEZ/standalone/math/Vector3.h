/*
	Vector3.h
*/

#pragma once

#ifndef _koala_Vector3_h__
#define _koala_Vector3_h__

#include "../koala.h"
#include "kmath.h"

NAMESPACE_KOALA_

struct Vector3
{
	float_t x, y, z;

	enum { kNumElements = 3 };
	typedef float_t value_type;

	Vector3() {}
	Vector3( float_t a ) : x(a), y(a), z(a) {}
	Vector3( float_t a, float_t b, float_t c ) : x(a), y(b), z(c) {}

	void set( float_t a, float_t b, float_t c ) { x = a; y = b; z = c; }
	float_t&       operator[] (int i)       { return *((&x) + i); }
	const float_t& operator[] (int i) const { return *((&x) + i); }
	operator const float_t* ()             const { return &x; }

	Vector3  operator *  ( float_t f ) const { return Vector3(x * f, y * f, z * f); }
	Vector3& operator *= ( float_t f ) { x *= f; y *= f;  z *= f; return *this; }
	Vector3  operator *  ( const Vector3 &v ) const { return Vector3(x * v.x, y * v.y, z * v.z); }
	Vector3& operator *= ( const Vector3 &v ) { x *= v.x; y *= v.y;  z *= v.z; return *this; }
	Vector3  operator /  ( float_t f ) const { return Vector3(x / f, y / f, z / f); }
	Vector3& operator /= ( float_t f ) { x /= f; y /= f;  z /= f; return *this; }
	Vector3  operator /  ( const Vector3 &v ) const { return Vector3(x / v.x, y / v.y, z / v.z); }
	Vector3& operator /= ( const Vector3 &v ) { x /= v.x; y /= v.y;  z /= v.z; return *this; }
	Vector3  operator +  ( float_t f ) const { return Vector3(x + f, y + f, z + f); }
	Vector3& operator += ( float_t f ) { x += f; y += f;  z += f; return *this; }
	Vector3  operator +  ( const Vector3 &v ) const { return Vector3(x + v.x, y + v.y, z + v.z); }
	Vector3& operator += ( const Vector3 &v ) { x += v.x; y += v.y;  z += v.z; return *this; }
	Vector3  operator -  ( float_t f ) const { return Vector3(x - f, y - f, z - f); }
	Vector3& operator -= ( float_t f ) { x -= f; y -= f;  z -= f; return *this; }
	Vector3  operator -  ( const Vector3 &v ) const { return Vector3(x - v.x, y - v.y, z - v.z); }
	Vector3& operator -= ( const Vector3 &v ) { x -= v.x; y -= v.y;  z -= v.z; return *this; }
	bool     operator == ( const Vector3 &v ) const { return x==v.x && y==v.y && z==v.z; }
	bool     operator != ( const Vector3 &v ) const { return x!=v.x || y!=v.y || z!=v.z; }

	// Negate
	Vector3  operator - () const { return Vector3(-x, -y, -z); }

	// Also known as the absolute value or magnitude of the vector
	float_t length() const { return math::sqrt(x * x + y * y + z * z); }

	// Same as this dot this, length() squared
	float_t lengthSquared() const { return x * x + y * y + z * z; }

	// Same as (this-v).length()
	float_t distanceBetween( const Vector3 &v ) const
	{
		return math::sqrt((x - v.x) * (x - v.x) + (y - v.y) * (y - v.y) + (z - v.z) * (z - v.z));
	}

	// Same as (this-v).lengthSquared()
	float_t distanceSquared( const Vector3 &v ) const
	{
		return (x - v.x) * (x - v.x) + (y - v.y) * (y - v.y) + (z - v.z) * (z - v.z);
	}

	// Dot product. Twice the area of the triangle between the vectors.
	float_t dot( const Vector3 &v ) const { return x * v.x + y * v.y + z * v.z; }

	// Cross product. This is a vector at right angles to the vectors with a length equal to the product of the lengths.
	Vector3 cross( const Vector3 &v ) const
	{
		return Vector3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
	}

	// Change the vector to be unit length. Returns the original length.
	float_t normalize()
	{
		const float_t d = length();
		if ( d )
			*this *= (float_t(1.0)/d);
		return d;
	}

	Vector3 normalized() const
	{
		const float_t d = length();
		return d ? *this * (float_t(1.0)/d) : *this;
	}

	float_t distanceFromPlane( float_t A, float_t B, float_t C, float_t D ) const
	{
		return A * x + B * y + C * z + D;
	}

	Vector3 reflect( const Vector3 &N )
	{
		return N * (dot(N) * float_t(2)) - *this;
	}

	// Mainly for zero
	// bool operator == ( float_t v ) const { return x==v && y==v && z==v; }
	// bool operator != ( float_t v ) const { return x!=v || y!=v || z!=v; }

	float_t sum()     const { return x+y+z; }
	Vector3 inverse() const { return Vector3(math::inverse(x), math::inverse(y), math::inverse(z)); }
	Vector3 squared() const { return Vector3(math::square(x), math::square(y), math::square(z)); }

	bool allAre ( const unsigned v ) const { return x==v && y==v && z==v; }
	static Vector3 zero() { return Vector3(0,0,0); }
};

_NAMESPACE_KOALA

#endif /* _koala_Vector3_h__ */
