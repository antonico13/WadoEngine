#ifndef WADO_GRAPHICS_ABSTRACTION_LAYER_PIPELINE
#define WADO_GRAPHICS_ABSTRACTION_LAYER_PIPELINE

#include "WdCommon.h"
#include "WdImage.h"
#include "WdBuffer.h"

#include "Shader.h"

#include "MainClonePtr.h"

#include "spirv.h"
#include "spirv_cross.hpp"

#include <string>
#include <vector>
#include <map>

namespace Wado::GAL {
    // Forward declaration
    class WdLayer;
    class WdRenderPass;

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
        //WdShaderResource& operator=(const WdShaderResource& other);
        //WdShaderResource(const WdShaderResource& other);
        // move assignment & constructor 
        //WdShaderResource& operator=(const WdShaderResource& other);
        //WdShaderResource(const WdShaderResource& other);
    };
    

    // WdPipeline along with the base resource types is the one 
    // class that is not abstracted away. setting shader params and 
    // creating the maps seems like it should be completely platform 
    // and API agnostic 
    class WdPipeline final {
        public:
            friend class WdLayer;
            friend class WdRenderPass;

            //friend class Vulkan::VulkanLayer;
            //friend class Vulkan::VulkanRenderPass;

            static const int UNIFORM_END = -1;

            // subpass inputs handled in setUniform 
            void setUniform(const WdStage stage, const std::string& paramName, const WdShaderResource& resource);

            // for array params
            void setUniform(const WdStage stage, const std::string& paramName, const std::vector<WdShaderResource>& resources);

            void addToUniform(const WdStage stage, const std::string& paramName, const WdShaderResource& resource, int index = UNIFORM_END);

            // These resources need to be handled separately compared to uniforms
            void setFragmentOutput(const std::string& paramName, Memory::WdClonePtr<WdImage> resource);

            void setDepthStencilResource(Memory::WdClonePtr<WdImage> resource);

        private:
            // Util types 
            enum WdShaderParameterType {
                WD_SAMPLED_IMAGE = 0x00000000, // sampler2D
                WD_TEXTURE_IMAGE = 0x00000001, // just texture2D
                WD_STORAGE_IMAGE = 0x00000002, // read-write
                WD_SAMPLED_BUFFER = 0x00000004,
                WD_BUFFER_IMAGE = 0x00000008,
                WD_UNIFORM_BUFFER = 0x00000010,
                WD_SUBPASS_INPUT = 0x00000020, // only supported by Vulkan
                WD_STORAGE_BUFFER = 0x00000040, 
                WD_STAGE_OUTPUT = 0x00000080, // used for outs 
                WD_SAMPLER = 0x00000100,
                WD_PUSH_CONSTANT = 0x00000200, // only supported by Vulkan backend
                WD_STAGE_INPUT = 0x00000400, // used for ins 
            };

            using WdVertexInput = struct WdVertexInput {
                WdShaderParameterType paramType;
                uint8_t decorationLocation;
                uint8_t decorationBinding; // I think this should be unused for now 
                uint8_t offset;
                uint8_t size;
                WdFormat format;
            };

            using WdUniform = struct WdUniform {
                WdShaderParameterType paramType;
                uint8_t decorationSet;
                uint8_t decorationBinding;
                uint8_t decorationLocation;
                uint8_t resourceCount;
                WdStageMask stages;
                std::vector<WdShaderResource> resources;
            };

            using WdSubpassInput = struct WdSubpassInput { // Vulkan only 
                WdShaderParameterType paramType; // always subpass input, doesn't really matter 
                uint8_t decorationSet;
                uint8_t decorationBinding;
                uint8_t decorationLocation;
                uint8_t decorationIndex;
                Memory::WdClonePtr<WdImage> resource;
            }; 

            using WdFragmentOutput = struct WdFragmentOutput { 
                WdShaderParameterType paramType; // always stage output, doesn't really matter 
                uint8_t decorationIndex; // Needed in order to create refs and layouts
                Memory::WdClonePtr<WdImage> resource; 
            }; 

            using WdUniformAddress = std::tuple<uint8_t, uint8_t, uint8_t>; // (Set, Binding, Location)
            using WdUniformIdent = std::tuple<std::string, WdStage>; // (uniform name, stage its in)

            using WdVertexInputs = std::vector<WdVertexInput>;
            using WdUniforms = std::map<WdUniformAddress, WdUniform>;
            using WdUniformAddresses = std::map<WdUniformIdent, WdUniformAddress>;
            using WdSubpassInputs = std::map<std::string, WdSubpassInput>;
            using WdFragmentOutputs = std::map<std::string, WdFragmentOutput>;

            WdPipeline(Shader::WdShaderByteCode vertexShader, Shader::WdShaderByteCode fragmentShader, const WdViewportProperties& viewportProperties);

            // Util functions for building up the parameters and pipeline properties
            void addUniformDescription(spirv_cross::Compiler& spirvCompiler, const spirv_cross::SmallVector<spirv_cross::Resource>& resources, const WdShaderParameterType paramType, const WdStage stage);

            void generateVertexParams();
            void generateFragmentParams();
            
            Shader::WdShaderByteCode _vertexShader;
            Shader::WdShaderByteCode _fragmentShader;

            WdUniformAddresses _uniformAddresses;
            WdUniforms _uniforms;

            WdVertexInputs _vertexInputs;
            WdSubpassInputs _subpassInputs; // fragment & Vulkan only
            WdFragmentOutputs _fragmentOutputs;

            Memory::WdClonePtr<WdImage> _depthStencilResource; // by default should be an invalid pointer?

            const WdViewportProperties& _viewportProperties;
    };    
}

#endif