/*
	CameraController.h
*/

#pragma once

#ifndef _koala_CameraController_h__
#define _koala_CameraController_h__

#include "koala.h"
#include "math/Matrix4.h"
#include "math/Vector2.h"
#include "math/Box.h"

NAMESPACE_KOALA_

class Camera;

class CameraController
{
public:
	enum { None, Track, Tumble, Dolly };
	enum { Orthographic, Perspective };
	struct MotionState;

	CameraController( Camera *camera = NULL );
	~CameraController();

	void setCamera( Camera &camera );
	const Camera* camera() const  { return mCamera; }

	// Updates the camera position based on a changed mouse position. Order of calling must be motionStart, motionUpdate ( optional motionEnd )
	void motionStart( unsigned motion, const Vector2i &startPosition );
	void motionUpdate( const Vector2i &newPosition );
	void motionEnd( const Vector2i &endPosition );

	void home();
	void focusOn( const Vector3 &pos = (0) );

	// Moves the camera to frame the specified box, keeping the current viewing direction unchanged.
	void frame( const Box3 &box );

	// Moves the camera to frame the specified box, viewing it from the  specified direction, and with the specified up vector.
	void frame( const Box3 &box, const Vector3 &viewDirection, const Vector3 &upVector = Vector3( 0, 1, 0 ) );
	

	// Computes the points on the near and far clipping planes that correspond with the specified raster position. Points are computed in world space.
	void unproject( const Vector2i rasterPosition, Vector3 &near, Vector3 &far ) const;

	// Projects the point in world space into a raster space position.
	Vector2 project( const Vector3 &worldPosition ) const;


	void getTransformations( const Matrix4 &proj, Matrix4 *view = NULL,
                  const Matrix4 *modelIN=0, Matrix4 *modelView = NULL, Matrix4 *modelViewProj = NULL,
                  Vector3 *eyeP = NULL ) const;

	void getTransformations( Matrix4 *proj = 0, Matrix4 *view = NULL,
                  const Matrix4 *modelIN=0, Matrix4 *modelView = NULL, Matrix4 *modelViewProj = NULL,
                  Vector3 *eyeP = NULL ) const;

	bool            perspective() const;
	const Matrix4&  transform()   const;
	const Matrix4   viewMatrix()  const;

	// These can differ from camera as CameraController can draw into a view not matching it's cameras resolution
	float_t fov()                 const;
	float_t focalLength()         const;
	const Vector2i& viewPort() const      { return mViewport; }
	void viewPort(unsigned x, unsigned y) { mViewport.set(x,y); }
    const float_t aspectRatio()   const   { return mViewport.aspectRatio(); }
	Matrix4   pixelProjection()   const   { return Matrix4(viewPort()); }
    Matrix4   projectionMatrix()  const;

	void viewportMask( float_t verts[16], unsigned elem[12] );
	Vector4 screenWindow() const;

private:

	void dispose();

	Camera       *mCamera;
    Vector2i     mViewport;
	Vector3      mHome;
	float_t      mCentreOfInterest;
	MotionState  *mState;
};

_NAMESPACE_KOALA

#endif /* _koala_CameraController_h__ */
