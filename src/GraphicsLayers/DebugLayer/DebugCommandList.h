#ifndef WADO_DEBUG_COMMAND_LIST
#define WADO_DEBUG_COMMAND_LIST

#include "WdRenderPass.h" // Not sure if this include is needed 
#include "WdCommandList.h"


namespace Wado::GAL::Debug {
    class DebugLayer;

    class DebugCommandList : public WdCommandList {
        public:
            friend class DebugLayer;

            void resetCommandList() override;
            void beginCommandList() override;
            void setRenderPass(const WdRenderPass& renderPass) override;
            void nextPipeline() override;
            void setVertexBuffers(const std::vector<WdBuffer>& vertexBuffer) override;
            void setIndexBuffer(const WdBuffer& indexBuffer) override;
            void setViewport(const WdViewportProperties& WdViewportProperties) override;
            void drawIndexed(uint32_t indexCount) override;
            void drawVertices(uint32_t vertexCount) override;
            void endRenderPass() override;
            void endCommandList() override;
            // non-immediate versions 
            void copyBufferToImage(const WdBuffer& buffer, const WdImage& image, WdExtent2D extent) override;
            void copyBuffer(const WdBuffer& srcBuffer, const WdBuffer& dstBuffer, WdSize size) override;
        private:
            DebugCommandList(); 
            
    };
};

#endif