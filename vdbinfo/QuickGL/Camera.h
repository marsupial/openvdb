/*
	Camera.h
*/

#pragma once

#ifndef _koala_Camera_h__
#define _koala_Camera_h__

#include "koala.h"
#include "math/Matrix4.h"
#include "math/Box.h"

NAMESPACE_KOALA_

class Camera
{
	Matrix4 buildProjectionMatrix(float fov, float ratio, float nearP, float farP) const;

public:
	// If screenWindow is specified as an empty box (the default),
	// then it'll be calculated to be -1,1 in width and to preserve
	// aspect ratio (based on resolution) in height.
	Camera( const Vector3   &pos = Vector3(5,5,5),
	        const Vector2i  &resolution = Vector2i(1280, 720),
	        float_t         fov = 50,
	        const Vector2f  &clippingPlanes = Vector2f(0.1, 100000000),
	        const Box2f     &screenWindow = Box2f() );

	Camera( const Matrix4   &transform,
	        float_t         fov = 50,
	        const Vector2i  &resolution = Vector2i(1280, 720),
	        const Vector2f  &clippingPlanes = Vector2f(0.1, 100000000),
	        const Box2f     &screenWindow = Box2f() );

	// Specifies the transform of the camera relative to the world.
	void transform( const Matrix4 &m ) { mTransform = m; }
	const Matrix4& transform() const   { return mTransform; }

	void resolution( const Vector2i &r, bool fitHoriz = true );
	void resolution( int w, int h, bool fitH = true ) { return resolution(Vector2i(w,h), fitH); }
	const Vector2i& resolution() const { return mResolution; }

	void screenWindow( const Box2f &s ) { mScreenWindow = s; }
	const Box2f& screenWindow() const   { return mScreenWindow; }

	void clippingPlanes( const Vector2f &c ) { mClippingPlanes = c; }
	const Vector2f& clippingPlanes() const   { return mClippingPlanes; }

	Matrix4  viewMatrix() const { return mTransform.inverse(); }
	Matrix4  projectionMatrix(float nearP, float farP) const;
	Matrix4  projectionMatrix() const;

	float_t& fov()         { return mFov; }
	float_t  fov()         const { return mFov; }
	bool&    perspective() { return mPerspective; }
	bool     perspective() const { return mPerspective; }
	float_t focalLength()  const ;

	void    resetScreen( const Vector2i &resolution );
	Vector2 project( const Vector3 &worldPosition ) const;
	void    unproject( const Vector2i rasterPosition, Vector3 &near, Vector3 &far ) const;

/*
	/// These functions provide basic information about the
	/// current OpenGL camera based entirely on the current
	/// graphics state.

	/// The matrix that converts from the current GL object space to camera space (the GL_MODELVIEW_MATRIX).
	static Matrix4 matrix();

	// The projection matrix (the GL_PROJECTION_MATRIX).
	static Matrix4 projectionMatrix();

	// True if the camera performs a perspective projection.
	static bool perspectiveProjection();

	// Calculates the position of the current GL camera relative to the current GL transform.
	static Vector3 positionInObjectSpace();

	// Calculates the view direction of the current GL camera relative to the current GL transform.
	static Vector3 viewDirectionInObjectSpace();

	// Calculates the up vector of the current GL camera relative to the current GL transform.
	static Vector3 upInObjectSpace();
*/

protected:
	
	Matrix4    mTransform;
	Vector2i   mResolution;
	Box2f      mScreenWindow;
	Vector2f   mClippingPlanes;

	float_t    mFov;
	bool       mPerspective;

};

_NAMESPACE_KOALA

#endif /* _koala_Camera_h__ */
