#ifndef WADO_GRAPHICS_ABSTRACTION_LAYER_COMMON
#define WADO_GRAPHICS_ABSTRACTION_LAYER_COMMON

#include <cstdint>
#include <vector>

namespace Wado::GAL {
    
    using WdResourceID = uint32_t;

    using WdHandle = void *;
    static const WdHandle WD_INVALID_HANDLE = nullptr;

    // Common types
    using WdFenceHandle = WdHandle; 
    using WdSemaphoreHandle = WdHandle;
    
    using WdMemoryHandle = WdHandle;
    using WdRenderTarget = WdHandle; 

    using WdSize = size_t;

    using WdFormatFeatureFlags = uint32_t;

    // basically 1-to-1 with Vulkan.
    // These flags can be queried and together with hardware details
    // it can be reasoned whether the render target of an image or
    // texel buffer is suitable for the features described in the enum.
    // Conversely, if a specific feature is required, the GPU can be 
    // queried for which WdFormat from above will enable the feature mask.
    enum WdFormatFeatures {
        WD_FORMAT_FEATURE_SAMPLED_IMAGE = 0x00000001,
        WD_FORMAT_FEATURE_STORAGE_IMAGE = 0x00000002,
        WD_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC = 0x00000004,
        WD_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER = 0x00000008,
        WD_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER = 0x00000010,
        WD_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC = 0x00000020,
        WD_FORMAT_FEATURE_VERTEX_BUFFER = 0x00000040,
        WD_FORMAT_FEATURE_COLOR_ATTACHMENT = 0x00000080,
        WD_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND = 0x00000100,
        WD_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT = 0x00000200,
        WD_FORMAT_FEATURE_BLIT_SRC = 0x00000400,
        WD_FORMAT_FEATURE_BLIT_DST = 0x00000800,
        WD_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR = 0x00001000,
    };

    using WdExtent2D = struct WdExtent2D {
        uint32_t width;  
        uint32_t height;
    };

    using WdExtent3D = struct WdExtent3D {
        uint32_t width;
        uint32_t height;
        uint32_t depth;
    }; 

    using WdViewportProperties = struct WdViewportProperties {
        WdExtent2D startCoords;
        WdExtent2D endCoords;

        struct DepthProperties {
            float min = 0.0f;
            float max = 1.0f;
        } depth;

        struct ScissorProperties {
            WdExtent2D offset;
            WdExtent2D extent;
        } scissor;
    };

    enum WdStage {
        Unknown = 0,
        Vertex = 1,
        Fragment = 2,
    };

    //using WdStageMask = uint32_t;
};

#endif