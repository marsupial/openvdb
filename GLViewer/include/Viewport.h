/*
	Viewport.h
*/

#ifndef VDB_Nuke_Viewport_h__
#define VDB_Nuke_Viewport_h__

#include "Camera.h"
#include "CameraController.h"
#include "CamMask.h"

NUKEVDB_NAMESPACE_

class CamMask;

class Viewport
{
protected:
    koala::Camera mCamera;
	koala::CameraController mCameraController;
	bool mMask;

    void grid(float size = 5.f) const;

public:
	enum Action {
	    kMouseDown = 1,
	    kMouseUp = 2,
	    kMouseMove = 4,
	    kScrollWheel = 8,
	    kRMouseModifier = 16,
	    kMMouseModifier = 32,
        kCmdKeyModifier = 64,
        kCtlKeyModifier = 128,
        kAltKeyModifier = 256,
	};

    Viewport() : mCamera(koala::Vector3(5, 5, 5), 35.f), mCameraController(&mCamera), mMask(true) {}
	void init(int width, int height);
	void resize(int width, int height);
	void render();
	void mouse(int x, int y, Action action);
};

_NUKEVDB_NAMESPACE

#endif
