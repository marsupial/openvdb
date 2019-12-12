/*
	SceneEditor.h
*/
#pragma once

#include <string>

#include "Matrix4.h"
#include "Vector2.h"
#include "Box.h"

class ITransformer;
using ToolContext = std::unique_ptr<ITransformer>;
struct UsdScene;

class SceneEditor {
public:
    struct SceneLoad {
        std::string file;
        std::string session;
        bool        zUp;
    };

    virtual ~SceneEditor() {}

    virtual void
    resize(unsigned width, unsigned height) = 0;
    
    virtual bool
    pick(Vector2i mouse, const ViewContext& view, bool forReal, ToolContext* = nullptr) = 0;

    virtual bool
    pick(Vector2i mouse, const ViewContext& view, Vector3& P) = 0;

    virtual void
    draw(const ViewContext& view) = 0;

    virtual bool
    open(SceneLoad& sd) {
        sd.zUp = false;
        return false;
    }

    virtual bool
    open(SceneLoad& sd, unsigned width, unsigned height, std::string renderer = {}) {
        return open(sd);
    }

    virtual bool
    init(unsigned width, unsigned height, std::string renderer = {}) {
        resize(width, height);
        return true;
    }

    virtual Box3
    bounds(bool selection = true, bool ignoreHints = true) const {
        return {};
    }

    virtual ToolContext
    toolContext() {
        return {};
    }

    virtual bool
    hasSelection() const {
        return false;
    }
    
    virtual bool
    clearSelection(bool forReal) {
        return false;
    }

    virtual std::vector<std::string>
    selection() {
        return {};
    }

    virtual void
    markDirty() {
    }

    virtual UsdScene*
    scene() {
        return nullptr;
    }

    virtual double
    setTime(double t) {
        return 0;
    }

    virtual double
    time() const {
        return 0;
    }

    virtual std::pair<double, double>
    timeRange() const {
        return {0,0};
    }

    virtual bool
    converged() const {
        return true;
    }

    static std::unique_ptr<SceneEditor> create(SceneLoad& sd);
};
