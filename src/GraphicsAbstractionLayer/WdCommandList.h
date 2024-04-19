#ifndef WADO_GRAPHICS_COMMAND_LIST
#define WADO_GRAPHICS_COMMAND_LIST

#include "WdCommon.h"
#include "WdImage.h"
#include "WdBuffer.h"

#include <vector>

namespace Wado::GAL { 
    // Forward declaration
    class WdLayer;
    class WdRenderPass;

    class WdCommandList {
        public:
            friend class WdLayer;
            //friend class Vulkan::VulkanLayer;

            virtual void resetCommandList() = 0;
            virtual void beginCommandList() = 0;
            virtual void setRenderPass(const WdRenderPass& renderPass) = 0;
            virtual void nextPipeline() = 0;
            virtual void setVertexBuffers(const std::vector<WdBuffer>& vertexBuffer) = 0;
            virtual void setIndexBuffer(const WdBuffer& indexBuffer) = 0;
            virtual void setViewport(const WdViewportProperties& viewportProperties) = 0;
            virtual void drawIndexed(uint32_t indexCount) = 0;
            virtual void drawVertices(uint32_t vertexCount) = 0;
            virtual void endRenderPass() = 0;
            virtual void endCommandList() = 0;
            // non-immediate versions 
            virtual void copyBufferToImage(const WdBuffer& buffer, const WdImage& image, WdExtent2D extent) = 0;
            virtual void copyBuffer(const WdBuffer& srcBuffer, const WdBuffer& dstBuffer, WdSize size) = 0;
        private:
            WdCommandList();
    };
}
#endif