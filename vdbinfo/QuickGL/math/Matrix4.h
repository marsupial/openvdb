/*
	Matrix4.h
*/

#pragma once

#ifndef _koala_Matrix4_h__
#define _koala_Matrix4_h__

#include "../koala.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

NAMESPACE_KOALA_

class Quaternion;

struct Matrix4
{
	enum RotationOrder { eXYZ = 0, eXZY, eYXZ, eYZX, eZXY, eZYX };
	const static Matrix4 kIdentity;

	static Matrix4 lookAt( const Vector3 &eye, const Vector3 &ctr, const Vector3 &up );
	static Matrix4 ortho( float_t left, float_t right, float_t bottom, float_t top, float_t zNear, float_t zFar);

	union { float_t m[4][4]; float_t m_[16]; };

	Matrix4() {}
	// Matrix4( const Matrix4 &mat ) { ::memcpy(m, mat.m, sizeof(m)); } #include <string.h>
	Matrix4( float_t x, float_t y = 0, float_t z = 0 )
	{
		set(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1);
	}
	Matrix4( float_t m00, float_t m01, float_t m02, float_t m03,
	         float_t m10, float_t m11, float_t m12, float_t m13,
	         float_t m20, float_t m21, float_t m22, float_t m23,
	         float_t m30 = 0, float_t m31 = 0, float_t m32 = 0, float_t m33 = 1)
	{
		set(m00, m01, m02, m03, m10, m11, m12, m13, m20, m21, m22, m23, m30, m31, m32, m33);
	}
    Matrix4( const Vector4 &v0, const Vector4 &v1, const Vector4 &v2, const Vector4 &v3 )
    {
      set(v0[0], v0[1], v0[2], v0[3],
          v1[0], v1[1], v1[2], v1[3],
          v2[0], v2[1], v2[2], v2[3],
          v3[0], v3[1], v3[2], v3[3]);
    }
    Matrix4( const Vector2i &vp ) // Screen ortho
    {
      set(2.f/vp.x, 0,           0, 0,
          0,        2.f/vp.y-0,  0, 0,
          0,        0,           1, 0,
          -1,       -1,          0, 1
      );
    }

	void set( float_t m00, float_t m01, float_t m02, float_t m03,
	          float_t m10, float_t m11, float_t m12, float_t m13,
	          float_t m20, float_t m21, float_t m22, float_t m23,
	          float_t m30, float_t m31, float_t m32, float_t m33 )
	{
		m[0][0] = m00, m[0][1] = m01, m[0][2] = m02, m[0][3] = m03;
		m[1][0] = m10, m[1][1] = m11, m[1][2] = m12, m[1][3] = m13;
		m[2][0] = m20, m[2][1] = m21, m[2][2] = m22, m[2][3] = m23;
		m[3][0] = m30, m[3][1] = m31, m[3][2] = m32, m[3][3] = m33;
	}

	Matrix4 inverse() const;
	Matrix4 concatenate( const Matrix4& ) const;
	float_t determinant() const;
	static void multiply( const Matrix4 &a, const Matrix4 &b, Matrix4 &c );

	inline float_t* operator [] ( int i )                  { kassert( i>=0 && i < 4 ) return m[i]; }
	inline const float_t* operator [] ( int i )      const { kassert( i>=0 && i < 4 ) return m[i]; }

	inline float_t* operator [] ( unsigned i )             { kassert( i < 4 ) return m[i]; }
	inline const float_t* operator [] ( unsigned i ) const { kassert( i < 4 ) return m[i]; }

#if defined(__LP64__) || defined(_WIN64)
	inline float_t* operator [] ( size_t i )               { kassert( i < 4 ) return m[i]; }
	inline const float_t* operator [] ( size_t i )   const { kassert( i < 4 ) return m[i]; }
#endif

	inline const float_t* array() const { return m[0]; }
	inline operator const float_t* () const { return array(); }

	inline Matrix4 operator  *  ( const Matrix4 &r ) const { Matrix4 tmp; multiply(*this, r, tmp); return tmp; }
	inline Matrix4& operator *= ( const Matrix4 &r )       { Matrix4 tmp; multiply(*this, r, tmp); *this = tmp; return *this; }

	Matrix4 operator +  ( const Matrix4& ) const;
	Matrix4 operator -  ( const Matrix4& ) const;
	bool    operator == ( const Matrix4& ) const;
	bool    operator != ( const Matrix4& ) const;

	Matrix4 operator * ( const float_t a ) const
	{
		return Matrix4( m[0][0] * a, m[0][1] * a, m[0][2] * a, m[0][3] * a, 
		                m[1][0] * a, m[1][1] * a, m[1][2] * a, m[1][3] * a,
		                m[2][0] * a, m[2][1] * a, m[2][2] * a, m[2][3] * a,
		                m[3][0] * a, m[3][1] * a, m[3][2] * a, m[3][3] * a );
	}

	Vector3 operator * ( const Vector3 &v ) const { return transform(v); }
	Vector4 operator * ( const Vector4 &v ) const { return transform(v); }

	Vector3 transform  ( const Vector3 &v ) const
	{
		return Vector3( m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z + m[3][0],
		                m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z + m[3][1],
		                m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z + m[3][2]);
	}

	Vector4 transform  ( const Vector4& v ) const
	{
		return Vector4( m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z + m[3][0] * v.w, 
		                m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z + m[3][1] * v.w,
		                m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z + m[3][2] * v.w,
		                m[0][3] * v.x + m[1][3] * v.y + m[2][3] * v.z + m[3][3] * v.w );
	}

	Vector3 ntransform(const Vector3& v) const
	{
		return Vector3( m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
		                m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
		                m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z );
	}

	Vector3 vtransform  ( const Vector3 &v ) const
	{
		return Vector3( m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z,
		                m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z,
		                m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z);
	}

	Matrix4 transposed() const
	{
		return Matrix4( m[0][0], m[1][0], m[2][0], m[3][0],
		                m[0][1], m[1][1], m[2][1], m[3][1],
		                m[0][2], m[1][2], m[2][2], m[3][2],
		                m[0][3], m[1][3], m[2][3], m[3][3] );
	}

	void makeIdentity()
	{
		m[1][0] = m[2][0] = m[3][0] = 0.0;
		m[0][1] = m[2][1] = m[3][1] = 0.0;
		m[0][2] = m[1][2] = m[3][2] = 0.0;
		m[0][3] = m[1][3] = m[2][3] = 0.0;
		m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0;
	}

	template <class T> void set( const T (&values)[4][4] )
	{
		for ( unsigned r = 0; r < 4; ++r )
		{
			for ( unsigned c = 0; c < 4; ++c )
				m[r][c] = values[r][c];
		}
	}
	template <class T> void get( T values[4][4] ) const
	{
		for ( unsigned r = 0; r < 4; ++r )
		{
			T *dst = values[r];
			const float_t *src = m[r];
			for ( unsigned c = 0; c < 4; ++c )
				dst[c] = src[c];
		}
	}

	Vector3 scale() const;
	const Vector3& translation() const { return *(Vector3*)(&m[3]); }

	Matrix4 orthoInverse() const;
	void decompose( Vector3 &position, Vector3 &scale, Quaternion &orientation ) const;

	void setTranslation( const Vector3& );
	void setScale( const Vector3& );
	void setScale( float_t f ) { return setScale(Vector3(f)); }

	void scale( const Vector3& );
	void translate( float_t x, float_t y, float_t z = 0 );
	void translate( const Vector3 &v ) { translate(v.x, v.y, v.z); }
	void rotate( const Vector3& );
	void setAxisAngle( const Vector3& axis, float_t angle );

	void multDirMatrix( const Vector3 &src, Vector3 &dst) const;

	Vector3 side()    const { return Vector3(m[0][0], m[1][0], m[2][0]); }
	Vector3 up()      const { return Vector3(m[0][1], m[1][1], m[2][1]); }
	Vector3 forward() const { return Vector3(m[0][2], m[1][2], m[2][2]); }
  
	static Matrix4 rotationMatrixWithUpDir( const Vector3 &fromDir, const Vector3 &toDir, const Vector3 &upDir );
	static void alignZAxisWithTargetDir( Vector3 targetDir, Vector3 upDir, Matrix4 &result );
};

_NAMESPACE_KOALA

#endif /* _koala_Matrix4_h__ */
