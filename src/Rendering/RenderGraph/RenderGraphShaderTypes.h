#ifndef WADO_RDG_TYPE_H
#define WADO_RDG_TYPE_H

#include <cstdint>

namespace Wado::RenderGraph {
    // Basic Vulkan-GLSL types 
    using RDResourceHandle = uint64_t;
    using RDTextureHandle = RDResourceHandle;
    using RDBufferHandle = RDResourceHandle;
    using RDSamplerHandle = RDResourceHandle;

    using RDCombinedTextureSampler = struct RDCombinedTextureSampler {
        RDTextureHandle texture;
        RDSamplerHandle sampler;
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