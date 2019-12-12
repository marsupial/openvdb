/*
	ViewContext.h
*/
#pragma once

#include "HandleShapes.h"
#include "ShaderProgram.h"

#include "Matrix4.h"
#include "Vector3.h"

#include <functional>

class CameraController;
class ShaderProgram;
class ViewContext;

class Constraint {
    unsigned mConstraint;
public:
    Constraint() : mConstraint(0) {}
    bool operator () (unsigned pick) { return (mConstraint = (pick & 0xffffff00)); }
    
    enum {
        kAxisX    = 0x01000000,
        kAxisY    = 0x00010000,
        kAxisZ    = 0x00000100,
        kAxisAll  = kAxisX | kAxisY | kAxisZ,
        kAxisView = kAxisAll << 4
    };
    bool applyX()  const { return mConstraint & kAxisX; }
    bool applyY()  const { return mConstraint & kAxisY; }
    bool applyZ()  const { return mConstraint & kAxisZ; }
    bool ignoreX() const { return !applyX(); }
    bool ignoreY() const { return !applyY(); }
    bool ignoreZ() const { return !applyZ(); }
    unsigned constrained() const { return mConstraint & 0xffffff00; }
    bool applyAll() const { return (mConstraint & kAxisAll) == kAxisAll; }
    
    static unsigned makeId(unsigned axisID, unsigned objID) {
        switch (axisID) {
            case 0: return kAxisX | objID;
            case 1: return kAxisY | objID;
            case 2: return kAxisZ | objID;
        }
        return axisID | objID;
    }
    
    static unsigned axisFromID(unsigned fullID) {
        switch (fullID & 0xffffff00) {
            case kAxisX: return 0;
            case kAxisY: return 1;
            case kAxisZ: return 2;
        }
        return 3;
    }
};

class Plane {
    Vector3 N;
    float   d;
public:
    Plane() = default;
    Plane(Vector3 n, const Vector3& P = {0,0,0}) {
        reset(n, P);
    }

    void
    reset(const Vector3& P) {
        d = P.dot(N);
    }
    
    void
    reset(Vector3 n, const Vector3& P) {
        N = n.normalized();
        d = P.dot(N);
    }

    Vector3
    project(const Vector3 &pt) const {
        return pt - N * (N.dot(pt) - d);
    }

    const Vector3&
    normal() const {
        return N;
    }

    bool
    intersect(const Vector3 &linePos, const Vector3 &lineDir, float &t) const;
    
    bool
    intersect(const Vector3 &near, const Vector3 &far, Vector3 &P) const;
};

class GLProgramScope {
    const ShaderProgram& mProgram;
    Matrix4 mProjection, mView;
public:
    GLProgramScope(const ShaderProgram& program, const ViewContext& ctx);
    GLProgramScope(const ShaderProgram& program, const ViewContext& ctx, const Matrix4& xform);
    void operator () (const Matrix4& model) const;

    template <class T> void uniform(const char *name, const T &value) const {
        return mProgram.uniform(name, value);
    }
};

class ViewContext {
    struct HandleCache {
        using UpdateFunction = std::function<Matrix4(const Matrix4& base, const Matrix4& xform, const Matrix4& view, float scale, unsigned axis)>;
        HandleCache(Matrix4 m, Vector4 c, UpdateFunction u) : xform(m), color(c), update(std::move(u)) {}
        
        Matrix4 xform;
        Vector4 color;
        UpdateFunction update;
    };
    CameraController&        mCameraController;
    Matrix4                  mViewOrientation;
    Vector3                  mViewTranslation;
    HandleScene              mHandles;
    std::vector<HandleCache> mHandleCache;

    UnitSphere               mUnitSphere;
    ShaderProgram            mHandleProgram;

    mutable float            mScale = 1.f;
    const float              mDisplayScale = 1.f;

    void
    updateView();
    
    std::pair<Vector3, Vector3>
    unproject(const Vector2i &mouse) const;

public:
    ViewContext(CameraController& cam, float displayScale);
    
    Matrix4
    updateView(const Matrix4& handleDraw, float scale);
    
    Matrix4
    draw(const Matrix4& handleDraw, float scale);
    
    void
    addTube(unsigned axis, unsigned object, float scale = 5.f);

    void
    addTube(HandleScene::HandleType terminator,
            unsigned axis, unsigned object, float scale = 5.f);
    
    void
    coneAxis(unsigned axis, unsigned object, float scale = 5);
    
    void
    cubeAxis(unsigned axis, unsigned object, float scale = 5);
    
    void
    centerSphere(unsigned object, float scale = 0.3f,
                 Vector4 Cf = {1.f,0.9f,0.3f,1.f},
                 unsigned axis = Constraint::kAxisAll);
    
    void
    torusAxis(unsigned axis, unsigned object, float scale = 5);
    
    void
    discAxis(unsigned axis, unsigned object, float scale = 5);

    void
    centerSphere(const Matrix4& xform, unsigned object, float s = 0.3f,
                 Vector4 Cf = {1.f,0.9f,0.3f,1.f}) const;

    bool
    worldPosition(const Vector2i &mouse, const Plane& plane, Vector3& P) const;
    
    void
    clear();

    std::pair<Matrix4, Matrix4>
    cameraAndProjection(bool zUpCompensation = true) const;

    Vector2u
    viewport() const;

    unsigned intersect(unsigned x, unsigned y) const;
    float setScale(const Vector3& P) const;

    Vector3 viewDirection()          const { return mViewOrientation.vtransform(Vector3(0,0,-1)); }
    float scale()                    const { return mScale; }
    float displayScale()             const { return mDisplayScale; }

};
