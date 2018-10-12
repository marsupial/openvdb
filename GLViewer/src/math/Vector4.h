/*
	Vector4.h
*/

#pragma once

#ifndef _koala_Vector4_h__
#define _koala_Vector4_h__

#include "../koala.h"
#include "Vector3.h"

NAMESPACE_KOALA_

struct Vector4
{
	float_t x, y, z, w;

	Vector4() {}
	Vector4( float_t a, float_t b, float_t c, float_t d = 1.0 ) : x(a), y(b), z(c), w(d) {}
	Vector4( const Vector3& v, float_t d = 1 ) : x(v.x), y(v.y), z(v.z), w(d) {}

	void set( float_t a, float_t b, float_t c, float_t d = 1 ) { x = a; y = b; z = c; w = d; }
	float_t&       operator[] (int i)       { return *((&x) + i); }
	const float_t& operator[] (int i) const { return *((&x) + i); }

	Vector4  operator *  ( float_t f ) const { return Vector4(x * f, y * f, z * f, w * f); }
	Vector4  operator *  ( const Vector4 &v ) const { return Vector4(x * v.x, y * v.y, z * v.z, w * v.w); }
	Vector4& operator *= ( float_t f ) { x *= f; y *= f;  z *= f; w *= f; return *this; }
	Vector4& operator *= ( const Vector4 &v ) { x *= v.x; y *= v.y;  z *= v.z; w *= v.w; return *this; }
	Vector4  operator /  ( float_t f ) const { return Vector4(x / f, y / f, z / f, w / f); }
	Vector4  operator /  ( const Vector4 &v ) const { return Vector4(x / v.x, y / v.y, z / v.z, w / v.w); }
	Vector4& operator /= ( float_t f ) { x /= f; y /= f;  z /= f; w /= f; return *this; }
	Vector4& operator /= ( const Vector4 &v ) { x /= v.x; y /= v.y;  z /= v.z; w /= v.w; return *this; }
	Vector4  operator +  ( float_t f ) const { return Vector4(x + f, y + f, z + f, w + f); }
	Vector4  operator +  ( const Vector4 &v ) const { return Vector4(x + v.x, y + v.y, z + v.z, w + v.w); }
	Vector4& operator += ( float_t f ) { x += f; y += f;  z += f; w += f; return *this; }
	Vector4& operator += ( const Vector4 &v ) { x += v.x; y += v.y;  z += v.z; w += v.w; return *this; }
	Vector4  operator -  ( float_t f ) const { return Vector4(x - f, y - f, z - f, w - f); }
	Vector4  operator -  ( const Vector4 &v ) const { return Vector4(x - v.x, y - v.y, z - v.z, w - v.w); }
	Vector4& operator -= ( float_t f ) { x -= f; y -= f;  z -= f; w -= f; return *this; }
	Vector4& operator -= ( const Vector4 &v ) { x -= v.x; y -= v.y;  z -= v.z; w -= v.w; return *this; }

	Vector4 premult()   const { return Vector4(x*w, y*w, z*w, 1); }
	float_t luminance() const { return (x+y+z) * float_t(1.0/3.0); }
	bool allAre ( const unsigned v ) const { return x==v && y==v && z==v && w==v; }

	Vector3 truncate() const { return Vector3(x,y,z); }
	// float_t dot( const Vector3 &v ) const { return x * v.x + y * v.y + z * v.z; }
	// float_t dot( const Vector4 &v ) const { return x * v.x + y * v.y + z * v.z + w * v.w; }
};

_NAMESPACE_KOALA

#endif /* _koala_Vector4_h__ */
