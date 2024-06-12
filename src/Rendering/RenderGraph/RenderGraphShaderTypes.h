#ifndef WADO_RDG_TYPE_H
#define WADO_RDG_TYPE_H

#include <cstdint>

namespace Wado::RenderGraph {
    // Basic Vulkan-GLSL types 
    using RDTextureHandle = uint64_t;
    using RDBufferHandle = uint64_t;
    using RDSamplerHandle = uint64_t;

    using RDCombinedTextureSampler = struct RDCombinedTextureSampler {
        const RDTextureHandle texture;
        const RDSamplerHandle sampler;

        private:
            RDCombinedTextureSampler(RDTextureHandle _textureHandle, RDSamplerHandle _samplerHandle) : texture(_textureHandle), sampler(_samplerHandle) { };
    };

    using RDReadWriteTexture = RDTextureHandle;

    using RDSampledTexture = RDTextureHandle;

    using RDSampledBuffer = RDBufferHandle;

    using RDReadWriteBuffer = RDBufferHandle;

    using RDSeparateSampler = RDSamplerHandle;

    // TODO: How to handle push constant? 

    using RDRenderTarget = RDTextureHandle;

    using RDSubpassInput = RDRenderTarget;

    // TODO: Uniform and SSBOs ??
};

#endif