/*
	Tool.h
*/
#pragma once

#include "Matrix4.h"
#include "Vector3.h"

class ViewContext;
class ITransformer;
using ToolContext = std::unique_ptr<ITransformer>;
struct ObjectXforms;

class Tool {
public:
    // Updates the camera position based on a changed mouse position. Order of calling must be motionStart, motionUpdate ( optional motionEnd )
    virtual ~Tool() {}

    virtual bool
    motionUpdate(const ViewContext&, const Vector2i &newPosition) = 0;

    virtual bool
    motionStart(const ViewContext&, const Vector2i &newPosition, unsigned pick = 0) { return false; }

    virtual bool
    motionFinish(const ViewContext& view, const Vector2i &endPosition)   { return motionUpdate(view, endPosition); }

    virtual bool
    motionStartFromDown(const ViewContext&, const Vector2i &newPosition) { return false; }

    virtual void
    draw(const ViewContext&, const Matrix4&) {}

    virtual bool
    setTransform(const ViewContext& view, ToolContext, ObjectXforms* = nullptr) { return false; }

    virtual bool
    update() const { return false; }

    virtual bool
    allowSelectionChange() const { return true; }

    virtual const Matrix4&
    editSpace() const { return Matrix4::kIdentity; }
};
