#ifndef WADO_GRAPHICS_ABSTRACTION_LAYER_SHADER_H
#define WADO_GRAPHICS_ABSTRACTION_LAYER_SHADER_H

#include <vector>
#include <cstdint>

#include "WdCommon.h"
#include "WdLayer.h"

namespace Wado::GAL {

    using WdShaderByteCode = const std::vector<uint8_t>;
    
    using WdShaderResourceLocation = struct WdShaderResourceLocation {
        uint64_t set;
        uint64_t binding; 
        // Only used for array bindings 
        uint64_t index;
    };

    class WdShader {
        public:
            // These are immediate-mode 
            // TODO: make sure to auto-infer layout 
            virtual void setShaderResource(const WdShaderResourceLocation location, const WdImageResourceHandle image) = 0;
            // For combined image-samplers 
            virtual void setShaderResource(const WdShaderResourceLocation location, const WdImageResourceHandle image, const WdSamplerHandle sampler) = 0;
            // For pure sampler resources 
            virtual void setShaderResource(const WdShaderResourceLocation location, const WdSamplerHandle sampler) = 0;

            virtual void setShaderResource(const WdShaderResourceLocation location, const WdBufferResourceHandle bufferResource) = 0;

            virtual void setShaderResource(const WdShaderResourceLocation location, const WdBuffer buffer, const WdSize offset = 0, const WdSize range = 0) = 0;
        private:
    };

    class WdVertexShader : public WdShader {
        public:

        private:
    };

    class WdFragmentShader : public WdShader {
        public:
            
        private:
    };
};

#endif