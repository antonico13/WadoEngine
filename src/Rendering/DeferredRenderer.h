#ifndef H_WD_RENDER_DEFERRED
#define H_WD_RENDER_DEFERRED

#include "Renderer.h"
#include "GraphicsAbstractionLayer.h"

#include <vector>

class Scene; // Forward declaration 

namespace Wado::Rendering {

class DeferredRender : public Renderer {
    public:
        DeferredRender(GAL::GraphicsLayer *graphicsLayer);
        void init();
        void render(Scene scene);
    private:
        std::shared_ptr<GAL::GraphicsLayer> _graphicsLayer; // can be shared if using multiple renderers at the same time
        std::unique_ptr<GAL::WdCommandList> _commandList; // unique command list per renderer 
        GAL::WdRenderPass _deferredRenderPass;
        GAL::WdViewportProperties _viewportProperties; // viewport is dynamic state, but keep global one for now
        GAL::WdFenceHandle frameInFlightFence; // only present when a frame has finished being rendered 

        std::vector<GAL::WdImage> deferredColorAttachments;
        GAL::WdImage depthAttachment;

        void createDeferredColorAttachments(GAL::WdFormat attachmentFormat = DEFAULT_ATTACHMENT_FORMAT);
        void createDepthAttachment();

        static const GAL::WdFormat DEFAULT_ATTACHMENT_FORMAT = WD_FORMAT_R8G8B8A8_SRGB;
        static const uint32_t DEFERRED_ATTACHMENT_COUNT = 4;

        GAL::WdExtent2D swapchainExtent;

        // will refactor later
        void drawFrame();

        void recordCommandList();
};      

};

#endif