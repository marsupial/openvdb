/*
	Viewport.h
*/
#pragma once

#include "Camera.h"
#include "CameraController.h"
#include "Vector2.h"

class Tool;
class ViewContext;
class SceneEditor;

class Viewport
{
protected:
    class FrameBuffer;
    class GLData;

    Camera                mCamera;
	CameraController      mCameraController;

    std::unique_ptr<GLData>      mGLData;
    std::unique_ptr<Tool>        mCurrentTool, mNavigationTool;
    std::unique_ptr<SceneEditor> mSceneEditor;

    bool mMask;

    void toolChange();

public:
    Viewport(const char* filepath = nullptr);
    ~Viewport();

	void init(int width, int height, float deviceScale);
	void resize(int width, int height);

    bool render();
    void drawBackground();
    void drawGrid();
    void drawScene();
    void drawMask();
    void drawTools();

    enum Action {
        kMouseDown = 1,
        kMouseUp = 2,
        kMouseMove = 4,
        kMouseEvents = kMouseDown | kMouseUp | kMouseMove,
        kScrollWheel = 8,
        kRMouseModifier = 16,
        kMMouseModifier = 32,
        kCmdKeyModifier = 64,
        kCtlKeyModifier = 128,
        kAltKeyModifier = 256,
    };
    struct Event {
        Vector2i mouse;
        Vector2u window;
        Action   action;
    };

    bool mouse(const Event&);
    bool keypress(char key);
    bool navigationEvent(const Event& e);
    SceneEditor* editor() { return mSceneEditor.get(); }
};
