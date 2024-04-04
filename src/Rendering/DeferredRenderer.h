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
        GAL::GraphicsLayer *_graphicsLayer;
        std::vector<GAL::WdImage> deferredColorAttachments;

        void createDeferredColorAttachments(GAL::WdFormat attachmentFormat = DEFAULT_ATTACHMENT_FORMAT);

        static const GAL::WdFormat DEFAULT_ATTACHMENT_FORMAT = WD_FORMAT_R8G8B8A8_SRGB;
        static const uint32_t DEFERRED_ATTACHMENT_COUNT = 4;

        GAL::WdExtent2D swapchainExtent;
};      

};

#endif