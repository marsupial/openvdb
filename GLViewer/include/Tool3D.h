/*
	Tool3D.h
*/
#pragma once

#include "Tool.h"
#include "ViewContext.h"

struct SceneEditor;

struct ObjectXforms {
    Matrix4 parent = Matrix4::kIdentity,
            object = Matrix4::kIdentity;
    Vector3 translate = Vector3(0,0,0),
            rotate    = Vector3(0,0,0),
            scale     = Vector3(1,1,1),
            pivot     = Vector3(0,0,0);
};

class ITransformer {
public:
    virtual ~ITransformer() {}
    virtual bool setTranslation(Vector3) = 0;
    virtual bool setScale(Vector3) = 0;
    virtual bool setRotation(Vector3) = 0;
    virtual bool setPivot(Vector3) = 0;
    virtual bool rebuild(ObjectXforms&) = 0;
    virtual bool commit()  = 0;
};

class Tool3D : public Tool {
protected:
    ToolContext mTransformer;

    Matrix4 mObjectToWorld, mWorldToObject, mObjectXform;
    Vector3 mWorldOrigin, mStartPos;
    
    Matrix4 mHandleToWorld, mHandleEdit, mHandleDraw;
    Vector3 mHandleScaling;
    
    float mScale = 1.f;
    Constraint mAxisConstraint;
    Plane mPlane;
    bool mEditing = false;
    bool mApplied = false;

    bool
    setTransform(ObjectXforms*);

    bool
    setTransform(const ViewContext& view, ToolContext xforms, ObjectXforms* = nullptr) override;

    Vector3
    axisConstraint(Vector3 v);

    Vector3
    choose(Vector3 a, Vector3 b, const Vector3& viewDirection) const;

    virtual
    bool setupPick(const Vector3& P, const ViewContext& view);

public:
    Tool3D(ViewContext& view, ToolContext xforms);

    bool
    motionFinish(const ViewContext& view, const Vector2i &mouse) override;

    bool
    motionUpdate(const ViewContext& view, const Vector2i &mouse) override;

    bool
    motionStart(const ViewContext& view, const Vector2i &mouse, unsigned picked) override;

    const Matrix4&
    editSpace() const override {
        return mHandleDraw;
    }

    virtual bool apply(Vector3, const Vector2i &mouse) = 0;

    static std::unique_ptr<Tool3D> translateTool(ViewContext& view, ToolContext xforms);
    static std::unique_ptr<Tool3D> scaleTool(ViewContext& view, ToolContext xforms);
    static std::unique_ptr<Tool3D> rotateTool(ViewContext& view, ToolContext xforms);
    static std::unique_ptr<Tool3D> dynamicsTool(ViewContext& view, ToolContext xforms, SceneEditor&);
    static std::unique_ptr<Tool>   timeTool(SceneEditor&);
};
