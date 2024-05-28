#ifndef WADO_GRAPHICS_RENDER_PASS
#define WADO_GRAPHICS_RENDER_PASS

#include <vector>

namespace Wado::GAL {
    // Forward declaration
    class WdPipeline;

    class WdRenderPass {
        public:
            friend class WdLayer;
            //friend class Vulkan::VulkanLayer;
        private:
            //const std::vector<WdPipeline>& _pipelines;
    };
};

#endif
