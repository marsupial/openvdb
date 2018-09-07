/*
CameraController.cpp
*/

#include "Camera.h"
#include "CameraController.h"
#include "math/kmath.h"

#include <limits>
#include <cmath>

NAMESPACE_KOALA_


bool CameraController::perspective()         const { kassert( mCamera != NULL ) return mCamera->perspective(); }
const Matrix4& CameraController::transform() const { kassert( mCamera != NULL ) return mCamera->transform(); }
const Matrix4 CameraController::viewMatrix() const { kassert( mCamera != NULL ) return mCamera->viewMatrix(); }

float_t CameraController::fov()      const
{
    const float_t vpAspect  = mViewport.aspectRatio(),
                  cmAspect = mCamera->resolution().aspectRatio();
    if ( vpAspect > cmAspect )
        return mCamera->fov() * cmAspect/vpAspect;
    return mCamera->fov();
}

float_t CameraController::focalLength()      const
{
    const float_t vpAspect  = mViewport.aspectRatio(),
                  cmAspect = mCamera->resolution().aspectRatio();
    if ( vpAspect > cmAspect )
        return mCamera->focalLength() * cmAspect/vpAspect;
    return mCamera->focalLength();
}

static void fitToCamResoultion( Box2f &window, float_t cAspect, int idx )
{
    const Vector2 ctr = window.center();
    window.min[idx] = (window.min[idx] - ctr[idx])*cAspect + ctr[idx];
    window.max[idx] = (window.max[idx] - ctr[idx])*cAspect + ctr[idx];
}

Vector4 CameraController::screenWindow() const
{
    Box2f   window = mCamera->screenWindow();
    float_t vAspect = mViewport.aspectRatio(),
            cAspect = mCamera->resolution().aspectRatio();
    if ( cAspect > vAspect )
        fitToCamResoultion(window, cAspect / vAspect, 1);
    else if ( vAspect > cAspect )
        fitToCamResoultion(window, vAspect / cAspect, 0);

    return Vector4(window.min.x, window.max.x, window.min.y, window.max.y);
}

Matrix4 CameraController::projectionMatrix() const
{
      const float_t  near = mCamera->clippingPlanes().x,
                   far = mCamera->clippingPlanes().y,
                   vpAspect = mViewport.aspectRatio();

    if ( mCamera->perspective() )
    {
      const float_t  fl = focalLength(),
                     rangeInv = ( 1.0f / ( near - far ) );

      return Matrix4(
              fl, 0, 0, 0,
              0, fl * vpAspect, 0, 0,
              0, 0, (near + far)*rangeInv, -1,
              0, 0, ((near*far)*rangeInv) * 2.f, 0
            );

    }

    Box2f   window = mCamera->screenWindow();
    float_t cAspect = mCamera->resolution().aspectRatio();
    if ( cAspect > vpAspect )
        fitToCamResoultion(window, cAspect / vpAspect, 1);
    else if ( vpAspect > cAspect )
        fitToCamResoultion(window, vpAspect / cAspect, 0);

    return Matrix4::ortho( window.min.x, window.max.x, window.min.y, window.max.y, near, far);
}

void CameraController::viewportMask( float_t vertsOUT[16], unsigned elemOUT[12] )
{
    kassert( mCamera != NULL );

    const Vector2i &vp      = viewPort();
    const float_t  vpAspect = vp.aspectRatio(),
                   cmAspect = mCamera->resolution().aspectRatio();

    float_t verts[] =
    {
        0,    0,
        float_t(vp.x), 0,
        float_t(vp.x), float_t(vp.y),
        0,    float_t(vp.y),

        0,    0,
        float_t(vp.x), 0,
        float_t(vp.x), float_t(vp.y),
        0,    float_t(vp.y),
    };

    if ( vpAspect < cmAspect )
    {
        float_t maskY = (vp.y - float_t(vp.x)/cmAspect) * 0.5;

        verts[5] = verts[7]   = maskY;
        verts[9] = verts[11]  = vp.y-maskY;
        verts[13] = verts[15] = vp.y;

    }
    else if ( vpAspect != cmAspect )
    {
        float_t maskX = (vp.x - float_t(vp.y)*cmAspect) * 0.5;

        verts[2] = verts[4]   = maskX;
        verts[8] = verts[14]  = vp.x-maskX;
        verts[10] = verts[12] = vp.x;

    }
    ::memcpy(vertsOUT, verts, sizeof(verts));

    if ( elemOUT )
    {
      const unsigned kElem[] =
      {
          0, 1, 2,
          0, 2, 3,
          4, 5, 6,
          4, 6, 7,
      };
      ::memcpy(elemOUT, kElem, sizeof(kElem));
    }
}

Vector2 CameraController::project( const Vector3 &worldPosition ) const
{
  return mCamera->project(worldPosition);
}

void CameraController::unproject( const Vector2i rasterPosition, Vector3 &near, Vector3 &far ) const
{
  return mCamera->unproject(rasterPosition, near, far);
}

void CameraController::getTransformations( const Matrix4 &proj, Matrix4 *view, const Matrix4 *modelIN,
                                           Matrix4 *modelView, Matrix4 *modelViewProj, Vector3 *eyeP ) const
{
  if ( view ) *view = mCamera->viewMatrix();
  if ( eyeP ) *eyeP = mCamera->transform().translation();

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

void CameraController::getTransformations( Matrix4 *proj, Matrix4 *view, const Matrix4 *modelIN,
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

struct CameraController::MotionState
{
    CameraController &mController;
    Vector2i         mMotionStart;
    Matrix4          mMotionMatrix;
    float            mMotionCentreOfInterest;
    Box2f            mMotionScreenWindow;

    MotionState( CameraController &ctrl, const Vector2i &pos ) :
        mController(ctrl), mMotionStart(pos), mMotionMatrix(ctrl.transform()), mMotionScreenWindow(mController.camera()->screenWindow()), mMotionCentreOfInterest(ctrl.mCentreOfInterest) {}

    CameraController& controller() { return mController; }

    bool       persp()      const { return mController.perspective(); }
    float_t    coi()        const { return mController.mCentreOfInterest; }
    float_t    fov()        const { return mController.fov(); }
    Vector2f   screen()     const { return mController.camera()->screenWindow().size(); }
    const Vector2i& viewPort() const { return mController.viewPort(); }

    void set( const Matrix4 &m )              { mController.mCamera->transform(m); }
    void set( const Matrix4 &m, float_t coi ) { mController.mCamera->transform(m); mController.mCentreOfInterest = coi; }
    void set( const Box2f &screenWindow )     { mController.mCamera->screenWindow(screenWindow); }

    virtual void update( const Vector2i &pos ) = 0;
    virtual void finish( const Vector2i &pos ) { update(pos); }
};

NAMESPACE_KPRIV_

struct TrackMotion : public CameraController::MotionState
{
    TrackMotion( CameraController &ctrl, const Vector2i &pos ) : MotionState(ctrl, pos) {}

    void update( const Vector2i &p )
    {
        const Vector2f speed = screen() * 2;
        const Vector2i d = p - mMotionStart;

        Vector3 translate( 0.0f );
        translate.x = -speed.x * float_t(d.x)/float_t(viewPort().x);
        translate.y = speed.y * float_t(d.y)/float_t(viewPort().y);
        if( persp() && fov() )
            translate *= tan( M_PI * fov() / 360.0f ) * coi();

        Matrix4 t = mMotionMatrix;
        t.translate( translate );
        set(t);
    }
};

struct TumbleMotion : public CameraController::MotionState
{
    TumbleMotion( CameraController &ctrl, const Vector2i &pos ) : MotionState(ctrl, pos) {}

    void update( const Vector2i &p )
    {
        const Vector3 centreOfInterestInWorld = mMotionMatrix.transform( Vector3( 0, 0, -coi() ) );

        Matrix4 t = Matrix4::kIdentity;
        t.translate( centreOfInterestInWorld );

        //const Vector3  &up = mController.camera()->transform().up();
        //const Vector2i d = (p - mMotionStart) * (up.y<0&&up.z<0 ? -1.f : 1.f);
        const Vector2i d = p - mMotionStart;

        if ( d.x )
            t.rotate( Vector3( 0, -d.x / 100.0f, 0 ) );

        if ( d.y )
        {
            Matrix4 xRotate;
            const Vector3 xAxisInWorld = mMotionMatrix.vtransform( Vector3(1,0,0) ).normalized();
            xRotate.setAxisAngle( xAxisInWorld, -d.y / 100.0f );
            t = xRotate * t;
        }

        t.translate( -centreOfInterestInWorld );

        set( mMotionMatrix * t );
    }
};

struct DollyMotion : public CameraController::MotionState
{
    DollyMotion( CameraController &ctrl, const Vector2i &pos ) : MotionState(ctrl, pos) {}

    void update( const Vector2i &p )
    {
        const float_t  kEpsilon = 0.000001;
    
        const Vector2i viewPort = this->viewPort();
        const Vector2f dv = Vector2f( (p - mMotionStart) ) / viewPort;
        float_t d = dv.x - dv.y;

        if ( persp() )
        {
            // 2.5 is a magic number that just makes the speed nice
            d *= 2.5f * mMotionCentreOfInterest;
            const float_t scale = mMotionCentreOfInterest - d;
            if (  scale > kEpsilon )
            {
                Matrix4 t = mMotionMatrix;
                t.translate( Vector3( 0, 0, -d ) );
                set(t, scale);
            }
        }
        else
        {
            // orthographic
            Box2f screenWindow = mMotionScreenWindow;

            const Vector2f centreNDC = Vector2f( mMotionStart ) / viewPort,
                           centre( math::lerp( screenWindow.min.x, screenWindow.max.x, centreNDC.x ),
                                   math::lerp( screenWindow.max.y, screenWindow.min.y, centreNDC.y ) );

            float_t scale = 1.0f - d;
            if ( scale > kEpsilon )
            {
                screenWindow.min = (screenWindow.min - centre) * scale + centre;
                screenWindow.max = (screenWindow.max - centre) * scale + centre;
                set(screenWindow);
            }
        }
    }
};

_NAMESPACE_KPRIV

void CameraController::motionStart( unsigned motion, const Vector2i &pos )
{
    dispose();

    switch( motion )
    {
        case Track:
            mState = new TrackMotion(*this, pos);
            break;
        case Tumble:
            mState = new TumbleMotion(*this, pos);
            break;
        case Dolly:
            mState = new DollyMotion(*this, pos);
            break;
        default :
            break;
    }
}

void CameraController::motionUpdate( const Vector2i &mouse )
{
    if ( mState ) mState->update(mouse);
}

void CameraController::motionEnd( const Vector2i &mouse )
{
    if ( mState )
    {
      mState->finish(mouse);
      dispose();
    }
}

void
CameraController::setCamera( Camera &camera )
{
    mCamera = &camera;
    mHome = camera.transform().translation();
    home();
}

CameraController::CameraController( Camera *camera ) : mCamera(camera), mState(NULL)
{
    if ( camera )
        setCamera(*camera);
}

void CameraController::dispose() { delete mState; mState = NULL; }

CameraController::~CameraController() { dispose(); }

void
CameraController::home()
{
    if ( fabs(mHome.normalized().y) == 1.f ) mHome.x += 0.00001f;

    mCamera->transform( Matrix4::lookAt(mHome, Vector3(0,0,0), Vector3(0,1,0)).inverse() );
    mCentreOfInterest = mHome.length();
    if ( !mCamera->perspective() )
        mCamera->resetScreen( mCamera->resolution() );
}

void CameraController::focusOn( const Vector3 &pos )
{
    mCentreOfInterest = mCamera->transform().translation().distanceBetween(pos);
}

void CameraController::frame( const Box3 &box, const Vector3 &viewDirection, const Vector3 &upVector )
{
    // make a matrix to centre the camera on the box, with the appropriate view direction
    Matrix4 cameraMatrix = Matrix4::rotationMatrixWithUpDir( Vector3( 0, 0, -1 ), viewDirection, upVector );
    Matrix4 translationMatrix = Matrix4::kIdentity;
    translationMatrix.translate( box.center() );
    cameraMatrix *= translationMatrix;

    // translate the camera back until the box is completely visible
    Matrix4 inverseCameraMatrix = cameraMatrix.inverse();
    Box3f cBox = box;
    cBox.transform(inverseCameraMatrix);

    const Vector2f &cp = mCamera->clippingPlanes();
    Box2f screenWindow = mCamera->screenWindow();
    if ( mCamera->perspective() )
    {
        // perspective. leave the field of view and screen window as is and translate
        // back till the box is wholly visible. this currently assumes the screen window
        // is centred about the camera axis.
        float z0 = cBox.size().x / screenWindow.size().x;
        float z1 = cBox.size().y / screenWindow.size().y;

        mCentreOfInterest = std::max( z0, z1 ) / tan( M_PI * mCamera->fov() / 360.0 ) + cBox.max.z + cp[0];
        cameraMatrix.translate( Vector3( 0.0f, 0.0f, mCentreOfInterest ) );
    }
    else
    {
        // orthographic. translate to front of box and set screen window
        // to frame the box, maintaining the aspect ratio of the screen window.
        mCentreOfInterest = cBox.max.z + cp[0] + 0.1; // 0.1 is a fudge factor
        cameraMatrix.translate( Vector3( 0.0f, 0.0f, mCentreOfInterest ) );

        float xScale = cBox.size().x / screenWindow.size().x;
        float yScale = cBox.size().y / screenWindow.size().y;
        float scale = std::max( xScale, yScale );

        Vector2f newSize = screenWindow.size() * scale;
        screenWindow.min.x = cBox.center().x - newSize.x / 2.0f;
        screenWindow.min.y = cBox.center().y - newSize.y / 2.0f;
        screenWindow.max.x = cBox.center().x + newSize.x / 2.0f;
        screenWindow.max.y = cBox.center().y + newSize.y / 2.0f;
    }

    mCamera->transform( cameraMatrix );
    mCamera->screenWindow( screenWindow );
}

void CameraController::frame( const Box3 &box )
{
    const Matrix4 &xform = transform();
    return frame( box, xform.vtransform(Vector3(0,0,-1)), xform.vtransform(Vector3(0,1,0)) );
}

_NAMESPACE_KOALA
