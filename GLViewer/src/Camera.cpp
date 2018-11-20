/*
	Camera.cpp
*/

#include "Camera.h"

NAMESPACE_KOALA_

void
Camera::resetScreen( const Vector2i &resolution )
{
	const float_t one = 1, aspectRatio = float_t(resolution.x)/float_t(resolution.y);
	if ( aspectRatio < 1.0f )
	{
		mScreenWindow.min.x = -one;
		mScreenWindow.max.x =  one;
		mScreenWindow.min.y = -one / aspectRatio;
		mScreenWindow.max.y =  one / aspectRatio;
	}
	else
	{
		mScreenWindow.min.y = -one;
		mScreenWindow.max.y =  one;
		mScreenWindow.min.x = -aspectRatio;
		mScreenWindow.max.x =  aspectRatio;
	}
}

Camera::Camera( const Matrix4   &transform,
                float_t         fov,
                const Vector2i  &resolution,
                const Vector2f  &clippingPlanes,
                const Box2f     &screenWindow ) :
	mTransform(transform), mFov(fov), mResolution(resolution), mScreenWindow(screenWindow), mClippingPlanes(clippingPlanes),
    mPerspective(1)
{
	resetScreen(resolution);
}

Camera::Camera( const Vector3   &pos,
                float_t         fov,
                const Vector2i  &resolution,
                const Vector2f  &clippingPlanes,
                const Box2f     &screenWindow ) :
	mFov(fov), mTransform(pos.x, pos.y, pos.z), mResolution(resolution), mScreenWindow(screenWindow), mClippingPlanes(clippingPlanes),
    mPerspective(1)
{
	resetScreen(resolution);
}

void
Camera::resolution( const Vector2i &resolution, bool fitHoriz )
{
	const float_t  oldAspect = float_t(mResolution.x)/float_t(mResolution.y),
	               newAspect = float_t(resolution.x)/float_t(resolution.y),
	               yScale = oldAspect / newAspect;

	// if ( mScreenWindow.isEmpty() ) resetScreen(resolution);
	float_t dx = float_t(mResolution.x)/float_t(resolution.x);

	if ( fitHoriz ) // keep horizonatal size
    {
      mScreenWindow.min.y *= yScale;
      mScreenWindow.max.y *= yScale;
    }
    else // keep vertical size
    {
      const Vector2f screenWindowCenter = mScreenWindow.center();
      const Vector2f scale = Vector2f(resolution) / Vector2f(mResolution);
      mScreenWindow.min = screenWindowCenter + (mScreenWindow.min - screenWindowCenter) * scale;
      mScreenWindow.max = screenWindowCenter + (mScreenWindow.max - screenWindowCenter) * scale;
    }

	mResolution = resolution;
	
	/*
	screenWindow.min.x *= yScale;
	screenWindow.max.x *= yScale;
	
	screenWindow.min.y *= dx;
	screenWindow.max.y *= dx;
	screenWindow.min.x *= dx;
	screenWindow.max.x *= dx;*/
}

static float_t
invFL( float_t mFov )
{
	return math::tan( math::degreesToRadians(mFov) * 0.5f );
}

float_t
Camera::focalLength() const
{
	return 1.0 / invFL(mFov);
}

void
Camera::unproject( const Vector2i rasterPosition, Vector3 &near, Vector3 &far ) const
{
	Vector2 ndc = Vector2f( rasterPosition ) / mResolution;
	Vector2 screen( math::lerp( mScreenWindow.min.x, mScreenWindow.max.x, ndc.x ), math::lerp( mScreenWindow.max.y, mScreenWindow.min.y, ndc.y ) );

	if( mPerspective )
	{
		const float_t d = invFL(mFov);
		const Vector3 camera( screen.x * d, screen.y * d, -1.0f );
		near = camera * mClippingPlanes[0];
		far = camera * mClippingPlanes[1];
	}
	else
	{
		near.set( screen.x, screen.y, -mClippingPlanes[0] );
		far.set( screen.x, screen.y, -mClippingPlanes[1] );
	}

	near = mTransform.transform(near);
	far = mTransform.transform(far);
}

Vector2
Camera::project( const Vector3 &worldPosition ) const
{
	Vector3 cameraPosition = viewMatrix() * worldPosition;

	if ( !mPerspective )
		return Vector2f( math::lerpfactor( cameraPosition.x, mScreenWindow.min.x, mScreenWindow.max.x ) * mResolution.x,
	                   math::lerpfactor( cameraPosition.y, mScreenWindow.max.y, mScreenWindow.min.y ) * mResolution.y );

	const float_t d = invFL(mFov);
	const Vector3 screenPosition = (cameraPosition / cameraPosition.z ) / d;

	return Vector2f( math::lerpfactor( screenPosition.x, mScreenWindow.max.x, mScreenWindow.min.x ) * mResolution.x,
		               math::lerpfactor( screenPosition.y, mScreenWindow.min.y, mScreenWindow.max.y ) * mResolution.y );
}
       
Matrix4 Camera::buildProjectionMatrix(float fov, float ratio, float near, float far) const
{
	if ( mPerspective )
    {
#if 0
      const float f = 1.0f / invFL(fov);
      const float rangeInv = ( 1.0f / ( near - far ) );
      return Matrix4(
        f, 0, 0, 0,
        0, f * ratio, 0, 0,
        0, 0, (near + far)*rangeInv, -1,
        0, 0, ((near*far)*rangeInv) * 2.f, 0
      );
#else
      const float tanHalfFovy = invFL(fov);
      const float rangeInv = ( 1.0f / ( far - near ) );
      return Matrix4(
        1.0 / (ratio*tanHalfFovy), 0, 0, 0,
        0, 1.0 / tanHalfFovy, 0, 0,
        0, 0, -(far+near)*rangeInv, -1,
        0, 0, -(2.0 * far * near) * rangeInv, 0
      );
#endif
  	}
    return Matrix4::ortho(mScreenWindow.min.x, mScreenWindow.max.x, mScreenWindow.min.y, mScreenWindow.max.y, near, far);
}
// ) * ( (near + far / ( near - far ) )

Matrix4 Camera::projectionMatrix(float near, float far) const
{
	const float_t ratio = float_t(mResolution[0]) / float_t(mResolution[1]);
	return buildProjectionMatrix(mFov, ratio, near, far);
}

Matrix4 Camera::projectionMatrix() const
{
	return projectionMatrix(mClippingPlanes[0], mClippingPlanes[1]);
}

void Camera::getTransformations( const Matrix4 &proj, Matrix4 *view, const Matrix4 *modelIN,
                                 Matrix4 *modelView, Matrix4 *modelViewProj, Vector3 *eyeP ) const
{
  if ( view ) *view = viewMatrix();
  if ( eyeP ) *eyeP = position();

  if ( modelView )
  {
      kassert( modelIN != NULL && view != NULL )
      *modelView = (*modelIN) * (*view );
    if ( modelViewProj )
        *modelViewProj = (*modelView) * proj;
  }
  else if ( modelViewProj )
  {
      kassert( modelIN != NULL && view != NULL  )
      *modelViewProj = (*modelIN) * (*view ) * proj;
  }
}

void Camera::getTransformations( Matrix4 *proj, Matrix4 *view, const Matrix4 *modelIN,
                                           Matrix4 *modelView, Matrix4 *modelViewProj, Vector3 *eyeP ) const
{
  if ( proj )
  {
      *proj = projectionMatrix();
      getTransformations(*proj, view, modelIN, modelView, modelViewProj, eyeP);
  }
  else
      getTransformations(projectionMatrix(), view, modelIN, modelView, modelViewProj, eyeP);
}

_NAMESPACE_KOALA
