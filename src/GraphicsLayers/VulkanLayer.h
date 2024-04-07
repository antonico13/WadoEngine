#ifndef H_WD_VULKAN_LAYER
#define H_WD_VULKAN_LAYER

#include "GraphicsAbstractionLayer.h"
#include <memory>

namespace Wado::GAL::Vulkan {
    class VulkanLayer : public GraphicsLayer {
        public:
            static std::shared_ptr<VulkanLayer> getVulkanLayer();
        private:
            VulkanLayer();
            static std::shared_ptr<VulkanLayer> layer;
            
    };
}

#endif