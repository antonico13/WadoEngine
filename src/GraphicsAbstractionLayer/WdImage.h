#ifndef WADO_GRAPHICS_ABSTRACTION_LAYER_IMAGE
#define WADO_GRAPHICS_ABSTRACTION_LAYER_IMAGE

#include "WdCommon.h"

#include <cstdint>

namespace Wado::GAL {
    // Forward declarations.
    // I think I should do this instead of so many includes
    class WdLayer; 

    // Image-specific type aliases 
    using WdImageHandle = WdHandle;
    using WdImageResourceHanlde = WdHandle;
    using WdSamplerHandle = WdHandle;

    using WdMipCount = uint64_t;
    using WdNumSamples = uint64_t;

    using WdImageSubpartsInfo = struct WdImageSubpartsInfo {
        WdImageAspectFlags aspects;
        WdMipCount startingMip;
        WdMipCount mipCount;
    };

    // Describes how an image's texels (~pixels) are laid out in memory.
    // To match Vulkan, linear represent row-major order. 
    // Always recommended to select optimal tiling, implementations
    // can then select the most efficient memory layout for images
    enum WdImageTiling { 
        WD_TILING_OPTIMAL = 0,
        WD_TILING_LINEAR = 1,
    };

    using WdImageUsageFlags = uint32_t;

    enum WdImageUsage {
        WD_UNDEFINED = 0x0,
        WD_TRANSFER_SRC = 0x00000001,
        WD_TRANSFER_DST = 0x00000002,
        WD_SAMPLED_IMAGE = 0x00000004,
        WD_STORAGE_IMAGE = 0x00000008,
        WD_COLOR_ATTACHMENT = 0x00000010,
        WD_DEPTH_STENCIL_ATTACHMENT = 0x00000020,
        WD_TRANSIENT_ATTACHMENT = 0x00000040,
        WD_INPUT_ATTACHMENT = 0x00000080,
        WD_PRESENT = 0x10000000, // special, Wd-only. This is used later on in the Vulkan backend for image layouts 
    };

    // Designed to match Vulkan 
    enum WdFormat {
        WD_FORMAT_UNDEFINED,
        WD_FORMAT_R8_UNORM,
        WD_FORMAT_R8_SINT,
        WD_FORMAT_R8_UINT,
        WD_FORMAT_R8_SRGB,
        WD_FORMAT_R8G8_UNORM,
        WD_FORMAT_R8G8_SINT,
        WD_FORMAT_R8G8_UINT,
        WD_FORMAT_R8G8_SRGB,
        WD_FORMAT_R8G8B8_UNORM,
        WD_FORMAT_R8G8B8_SINT,
        WD_FORMAT_R8G8B8_UINT,
        WD_FORMAT_R8G8B8_SRGB,
        WD_FORMAT_R8G8B8A8_UNORM,
        WD_FORMAT_R8G8B8A8_SINT,
        WD_FORMAT_R8G8B8A8_UINT,
        WD_FORMAT_R8G8B8A8_SRGB,
        WD_FORMAT_R16_SINT,
        WD_FORMAT_R16_UINT,
        WD_FORMAT_R16_SFLOAT,
        WD_FORMAT_R16G16_SINT,
        WD_FORMAT_R16G16_UINT,
        WD_FORMAT_R16G16_SFLOAT,
        WD_FORMAT_R16G16B16_SINT,
        WD_FORMAT_R16G16B16_UINT,
        WD_FORMAT_R16G16B16_SFLOAT,
        WD_FORMAT_R16G16B16A16_SINT,
        WD_FORMAT_R16G16B16A16_UINT,
        WD_FORMAT_R16G16B16A16_SFLOAT,
        WD_FORMAT_R32_SINT,
        WD_FORMAT_R32_UINT,
        WD_FORMAT_R32_SFLOAT,
        WD_FORMAT_R32G32_SINT,
        WD_FORMAT_R32G32_UINT,
        WD_FORMAT_R32G32_SFLOAT,
        WD_FORMAT_R32G32B32_SINT,
        WD_FORMAT_R32G32B32_UINT,
        WD_FORMAT_R32G32B32_SFLOAT,
        WD_FORMAT_R32G32B32A32_SINT,
        WD_FORMAT_R32G32B32A32_UINT,
        WD_FORMAT_R32G32B32A32_SFLOAT,
        WD_FORMAT_R64_SINT,
        WD_FORMAT_R64_UINT,
        WD_FORMAT_R64_SFLOAT,
        WD_FORMAT_R64G64_SINT,
        WD_FORMAT_R64G64_UINT,
        WD_FORMAT_R64G64_SFLOAT,
        WD_FORMAT_R64G64B64_SINT,
        WD_FORMAT_R64G64B64_UINT,
        WD_FORMAT_R64G64B64_SFLOAT,
        WD_FORMAT_R64G64B64A64_SINT,
        WD_FORMAT_R64G64B64A64_UINT,
        WD_FORMAT_R64G64B64A64_SFLOAT,
        WD_FORMAT_D32_SFLOAT,
        WD_FORMAT_D32_SFLOAT_S8_UINT,
        WD_FORMAT_D24_UNORM_S8_UINT,
    };

    // This enum describes how many samples to take per pixel when resolving 
    // an image. Before being rendered to screen, images are described in 
    // terms of "texels". The number of samples describe the relationship
    // between texels and screen pixels.
    enum WdSampleCount {
        WD_SAMPLE_COUNT_1 = 0x00000001,
        WD_SAMPLE_COUNT_2 = 0x00000002, 
        WD_SAMPLE_COUNT_4 = 0x00000004, 
        WD_SAMPLE_COUNT_8 = 0x00000008, 
        WD_SAMPLE_COUNT_16 = 0x00000010,
        WD_SAMPLE_COUNT_32 = 0x00000020,
        WD_SAMPLE_COUNT_64 = 0x00000040,
    };

    // Filter modes and address modes describe how a sampler
    // will extract information from a texture when read in a
    // shader. These are pretty much 1-to-1 with Vulkan.
    enum WdFilterMode {
        WD_NEAREST_NEIGHBOUR = 0,
        WD_LINEAR = 1,
    };

    enum WdAddressMode {
        WD_REPEAT = 0,
        WD_MIRROR_REPEAT = 1,
        WD_CLAMP_TO_EDGE = 2, 
        WD_CLAMP_TO_BORDER = 3,
    };

    // Textures are addressed in normalized u-v-w coordinates in Wado. 
    using WdTextureAddressMode = struct WdTextureAddressMode {
        WdAddressMode uMode;
        WdAddressMode vMode;
        WdAddressMode wMode; 
    };

    // Color & depth stencil values. These are used 
    // mainly when loading fragment shader outputs or 
    // subpass inputs in order to reset the values for the 
    // entire image 
    using WdColorValue = struct WdColorValue {
        float r;
        float g;
        float b;
        float a;
    };

    using WdDepthStencilValue = struct WdDepthStencilValue {
        float depth;
        uint32_t stencil;
    };

    using WdClearValue = union WdClearValue {
        WdColorValue color;
        WdDepthStencilValue depthStencil;
    };

    // Constants that are used as default values  
    const WdTextureAddressMode DEFAULT_TEXTURE_ADDRESS_MODE = {WdAddressMode::WD_REPEAT, WdAddressMode::WD_REPEAT, WdAddressMode::WD_REPEAT};
    const WdDepthStencilValue DEFAULT_DEPTH_STENCIL_CLEAR = {1.0f, 0};
    const WdColorValue DEFAULT_COLOR_CLEAR = {0.0f, 0.0f, 0.0f, 1.0f};

    using WdImage = struct WdImage {
        public:
            friend class WdLayer;

            const WdResourceID resourceID;
            // Extend for multiple frames in flight. 
            // If object does not need independent frame-wise copies, all of the elements in the vector are the same
            const std::vector<WdImageHandle> handles;
            const std::vector<WdMemoryHandle> memories;
            const std::vector<WdRenderTarget> targets;
            const WdFormat format;
            const WdExtent2D extent;
            const WdImageUsageFlags usage;
            const WdClearValue clearValue;
        private:
            WdImage(WdResourceID _resourceID, const std::vector<WdImageHandle>& _handles, const std::vector<WdMemoryHandle>& _memories, const std::vector<WdRenderTarget>& _targets, WdFormat _format, WdExtent2D _extent, WdImageUsageFlags _usage, WdClearValue _clearValue) : 
                resourceID(_resourceID),
                handles(_handles), 
                memories(_memories),
                targets(_targets),
                format(_format),
                extent(_extent),
                usage(_usage),
                clearValue(_clearValue) {};
    };

    using WdImageDescription = struct WdImageDescription {
        public:
            WdImageDescription(WdFormat _format, WdImageUsageFlags _flags, WdExtent2D _extent, WdClearValue _clearValue, WdMipCount _mipCount, WdSampleCount _sampleCount) : format(_format), flags(_flags), extent(_extent), clearValue(_clearValue), mipCount(_mipCount), sampleCount(_sampleCount) { };
            const WdFormat format;
            const WdImageUsageFlags flags;
            const WdExtent2D extent;
            const WdClearValue clearValue;
            const WdMipCount mipCount;
            const WdSampleCount sampleCount;
    };
};

#endif