/*
	HandleShapes.h
*/
#pragma once

#include "GLShapes.h"
#include "Matrix4.h"

#include <vector>
#include <unordered_map>

class IHandle {
public:
    virtual ~IHandle();
    virtual void draw() = 0;
    virtual bool intersect(Vector3 origin, Vector3 dir, float& t) = 0;
};

template <typename T>
class Handle : public IHandle {
protected:
    T mGLShape;
public:
    void draw() override {
        mGLShape();
    }
    template <typename ...Args> void init(Args && ...args) {
        return mGLShape.init(std::forward<Args>(args)...);
    }

    bool intersect(Vector3 origin, Vector3 dir, float& t) override = 0;
};

class HandleScene {
    template <typename T, typename ...Args> unsigned getHandleIndex(unsigned, Args... args);

    struct Handle {
        Matrix4         xform, inverse;
        const unsigned  index, handleID;

        Handle(Matrix4 m, unsigned id, unsigned i);
        void update(Matrix4 m) { xform = m; inverse = m.inverse(); }
    };
    std::unordered_map<unsigned, unsigned> mHandleMap;
    std::vector<std::unique_ptr<IHandle>> mHandleInstances;
    std::vector<Handle> mHandles;

public:
    enum HandleType {
        UnitQuad,
        UnitCube,
        UnitSphere,
        UnitCylinder,
        UnitCone,
        UnitDisk,
        UnitTorus
    };
    using iterator = decltype(mHandles)::iterator;

    iterator begin() { return mHandles.begin(); }
    iterator end()   { return mHandles.end(); }

    std::pair<IHandle&, Handle&> operator[](size_t i) {
        Handle& handle = mHandles[i];
        return { *mHandleInstances[handle.index], handle };
    }

    size_t size() const { return mHandles.size(); }
    bool empty()  const { return mHandles.empty(); }
    void clear()  { std::vector<Handle>().swap(mHandles); }

    void emplace_back(HandleType type, Matrix4 xform, unsigned handleID);
    unsigned intersect(Vector3 origin, Vector3 dir, Vector3* outPoint = nullptr) const;

    template <typename T> void
    forEach(T op) {
        for (auto& h : mHandles) {
            op(mHandleInstances[h.index], h);
        }
    }

    template <typename T> void
    update(T op) {
        for (auto& h : mHandles) {
            op(h.xform, h.handleID);
        }
    }
};

class QuadHandle : public Handle<UnitQuad> {
public:
    ~QuadHandle() override;
    bool intersect(Vector3 origin, Vector3 dir, float& t) override;
};

class CubeHandle : public Handle<UnitCube> {
public:
    ~CubeHandle() override;

    bool intersect(Vector3 origin, Vector3 dir, float& t) override;
};

class SphereHandle : public Handle<UnitSphere> {
public:
    ~SphereHandle() override;
    
    bool intersect(Vector3 origin, Vector3 dir, float& t) override;
};

class CylinderHandle : public Handle<UnitCylinder> {
public:
    ~CylinderHandle() override;
    
    bool intersect(Vector3 origin, Vector3 dir, float& t) override;
};

class ConeHandle : public Handle<UnitCone> {
public:
    ~ConeHandle() override;
    
    bool intersect(Vector3 origin, Vector3 dir, float& t) override;
};

class DiscHandle : public Handle<UnitDisk> {
protected:
    float mInnerSqr = 0,
          mOuterSqr = 1;
public:
    ~DiscHandle() override;
    
    void init(float outerRadius = 0.5f, float innerRadius = 0.4f, unsigned stacks = 30);

    bool intersect(Vector3 origin, Vector3 dir, float& t) override;
};

class TorusHandle : public Handle<UnitTorus> {
protected:
    float mInnerSqr    = 0.16f,
          mOuterSqr    = 0.25f,
          mSweepRadius = 0.45f,
          mTubeRadius  = 0.05f;
public:
    ~TorusHandle() override;
    
    void init(float outerRadius = 0.5f, float innerRadius = 0.4f, unsigned sweepDivisions = 30, unsigned tubedivisions = 15);

    bool intersect(Vector3 origin, Vector3 dir, float& t) override;
};

