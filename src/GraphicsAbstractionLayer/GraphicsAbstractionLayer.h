#ifndef WADO_GRAPHICS_ABSTRACTION_LAYER
#define WADO_GRAPHICS_ABSTRACTION_LAYER

#include <cstdint>

namespace Wado::GAL {

    using WdImageHandle = void *; // not sure about this here?
    using WdBufferHanlde = void *;

    using WdSize = size_t;

    using WdExtent2D = struct WdExtent2D {
        uint32_t height;
        uint32_t width;
    };

    using WdExtent3D = struct WdExtent3D {
        uint32_t height;
        uint32_t width;
        uint32_t depth;
    };

    enum WdFormat {
        WD_FORMAT_R8G8B8A8_UINT,
        WD_FORMAT_R8G8B8A8_SINT,
        WD_FORMAT_R8G8B8A8_SRGB,
        WD_FORMAT_UNDEFINED,
        WD_FORMAT_R32G32B32A32_UINT,
        WD_FORMAT_R32G32B32A32_SINT,
        WD_FORMAT_R32G32B32A32_SFLOAT,
    };

    enum WdBufferUsage {
        WD_TRANSFER_SRC,
        WD_TRANSFER_DST,
        WD_UNIFORM_BUFFER,
        WD_STORAGE_BUFFER,
        WD_INDEX_BUFFER,
        WD_VERTEX_BUFFER,
        WD_UNIFORM_TEXEL_BUFFER,
        WD_STORAGE_TEXEL_BUFFER,
        WD_INDIRECT_BUFFER,
    };

    
    enum WdImageUsage {
        WD_TRANSFER_SRC,
        WD_TRANSFER_DST,
        WD_SAMPLED_IMAGE,
        WD_STORAGE_IMAGE,
        WD_COLOR_ATTACHMENT,
        WD_DEPTH_STENCIL_ATTACHMENT,
        WD_INPUT_ATTACHMENT,
        WD_TRANSIENT_ATTACHMENT,
    };

    enum WdSampleCount {
        WD_SAMPLE_COUNT_1,
        WD_SAMPLE_COUNT_2, 
        WD_SAMPLE_COUNT_4, 
        WD_SAMPLE_COUNT_8, 
        WD_SAMPLE_COUNT_16,
        WD_SAMPLE_COUNT_32,
        WD_SAMPLE_COUNT_64,
    };

}

#endif