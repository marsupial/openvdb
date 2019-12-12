/*
	HydraViewport.h
*/
#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE
    class HdSceneDelegate;
    class HdRenderIndex;
    class UsdImagingGLRenderParams;
    class GfMatrix4d;
    class GfVec3d;
PXR_NAMESPACE_CLOSE_SCOPE

struct UsdScene;

class HydraViewport {
    struct Engine;
    struct Renderer;
    using  Renderers = std::unordered_map<std::string, std::string>;
protected:
    std::unique_ptr<Renderer> mRenderer;

    void
    setLighting(const PXR_NS::GfVec3d& position,
                bool ambient, bool dome,
                bool zUp = false);

public:
    static Renderers
    getRenderers();
    static const std::string kGLRenderer;

    HydraViewport();

    bool
    init(const std::string& renderer = {}, float t = 0.f);
    
    bool
    setRenderer(const std::string& renderer);

    void
    draw(UsdScene& scene, const PXR_NS::GfMatrix4d& xform, const PXR_NS::GfMatrix4d& proj);

    void
    setTime(double frame);

    bool
    converged() const;

    void
    resize(unsigned width, unsigned height);

    void
    addDelegate(std::unique_ptr<PXR_NS::HdSceneDelegate>);

    PXR_NS::HdRenderIndex*
    renderIndex();

    Renderer&
    renderer();

    struct DrawData;
};
