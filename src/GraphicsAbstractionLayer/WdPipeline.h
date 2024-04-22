#ifndef WADO_GRAPHICS_ABSTRACTION_LAYER_PIPELINE
#define WADO_GRAPHICS_ABSTRACTION_LAYER_PIPELINE

#include "WdCommon.h"
#include "WdImage.h"
#include "WdBuffer.h"

#include "MainClonePtr.h"

#include <string>
#include <vector>

namespace Wado::GAL {

    using WdImageResource = struct WdImageResource {
        Memory::WdClonePtr<WdImage> image;
        WdSamplerHandle sampler;
    };

    using WdBufferResource = struct WdBufferResource {
        Memory::WdClonePtr<WdBuffer> buffer;
        WdRenderTarget bufferTarget;
    };

    using WdShaderResource = union WdShaderResource {
        const WdImageResource imageResource;
        const WdBufferResource bufferResource;

        WdShaderResource(Memory::WdClonePtr<WdImage> img);
        WdShaderResource(Memory::WdClonePtr<WdBuffer> buf);
        WdShaderResource(WdSamplerHandle sampler);
        WdShaderResource(Memory::WdClonePtr<WdImage> img, WdSamplerHandle sampler);
        WdShaderResource(Memory::WdClonePtr<WdBuffer> buf, WdRenderTarget bufTarget);

        // copy assignment & constructor
        // WdShaderResource& operator=(const WdShaderResource& other);
        // WdShaderResource(const WdShaderResource& other);
        // move assignment & constructor 
        // WdShaderResource& operator=(const WdShaderResource& other);
        // WdShaderResource(const WdShaderResource& other);
    };
    
    class WdPipeline  {
        public:
            inline static const int UNIFORM_END = -1;

            virtual void setUniform(const WdStage stage, const std::string& paramName, const WdShaderResource& resource) = 0;

            virtual void setUniform(const WdStage stage, const std::string& paramName, const std::vector<WdShaderResource>& resources) = 0;

            virtual void addToUniform(const WdStage stage, const std::string& paramName, const WdShaderResource& resource, int index = UNIFORM_END) = 0;

            // These resources need to be handled separately compared to uniforms
            virtual void setFragmentOutput(const std::string& paramName, Memory::WdClonePtr<WdImage> image) = 0;

            virtual void setDepthStencilResource(Memory::WdClonePtr<WdImage> image) = 0;            
    };    
};

#endif