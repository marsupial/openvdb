/*
	USDViewport.h
*/
#pragma once

#include "HydraViewport.h"

#include <pxr/imaging/glf/glew.h>
#include <pxr/usdImaging/usdImagingGL/engine.h>
#include <pxr/usd/usd/stage.h>

struct UsdScene {
    PXR_NS::UsdStageRefPtr  stage;
    PXR_NS::UsdPrim         root;
    bool                    zUp;

    static std::unique_ptr<UsdScene>
    create(std::string scene, const std::string& sessionFilePath = {});

    UsdScene(PXR_NS::UsdStageRefPtr stg);
};

struct HydraViewport::Engine : public PXR_NS::UsdImagingGLEngine {
    friend struct HydraViewport::Renderer;
public:
    using UsdImagingGLEngine::UsdImagingGLEngine;
    PXR_NS::UsdImagingDelegate& delegate() { return *_delegate; }
    PXR_NS::HdRenderIndex* renderIndex() { return _renderIndex; }
    PXR_NS::HdChangeTracker& changeTracker()  { return _renderIndex->GetChangeTracker(); }
    PXR_NS::TfToken rendererID() const { return _rendererId; };
    PXR_NS::GlfSimpleLightingContextRefPtr lighting()  { return _lightingContextForOpenGLState; }
    PXR_NS::HdxTaskController* taskController() { return _taskController; }
    void removeObject(PXR_NS::SdfPath);
    void addObject(PXR_NS::SdfPath);
    void reinit();

};

struct HydraViewport::Renderer {
    Engine engine;
    PXR_NS::UsdImagingGLRenderParams params;

    std::pair<bool,bool>
    pick(float x, float y, float width, float height,
         UsdScene                  &scene,
         const PXR_NS::GfMatrix4d  &xform,
         const PXR_NS::GfMatrix4d  &proj,
         PXR_NS::SdfPath           &hitPrimPath,
         PXR_NS::SdfPath           *outHitInstancerPath = nullptr,
         int                       *outHitInstanceIndex = nullptr,
         int                       *outHitElementIndex = nullptr,
         PXR_NS::GfVec3d           *outHitPoint = nullptr,
         const HydraViewport       * = nullptr,
         const UsdScene            * = nullptr);
};

struct HydraViewport::DrawData {
    PXR_NS::HdEngine           &engine;
    PXR_NS::HdRenderIndex      *renderIndex;
    PXR_NS::HdxTaskController  *taskController;
    unsigned                   vao = 0;
    PXR_NS::HdRprimCollection  *collection = nullptr;
    PXR_NS::SdfPathVector      *roots = nullptr;
    
    void
    draw(PXR_NS::HdTaskSharedPtrVector tasks,
         const PXR_NS::UsdImagingGLRenderParams& params) const;
    
    bool
    testIntersection(const PXR_NS::GfMatrix4d &viewMatrix,
                     const PXR_NS::GfMatrix4d &projectionMatrix,
                     const PXR_NS::UsdImagingGLRenderParams& params,
                     PXR_NS::GfVec3d *outHitPoint,
                     PXR_NS::SdfPath *outHitPrimPath,
                     PXR_NS::SdfPath *outHitInstancerPath,
                     int *outHitInstanceIndex,
                     int *outHitElementIndex) const;
};
