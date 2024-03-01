#ifndef H_WD_SHADER
#define H_WD_SHADER

#include <cstdint>
#include <vector>

namespace Wado::Shader {

enum ShaderParameterType {
    WD_SAMPLED_IMAGE, // sampler2D
    WD_TEXTURE_IMAGE,// just texture2D
    WD_STORAGE_IMAGE, // read-write
    WD_SAMPLED_BUFFER,
    WD_BUFFER_IMAGE,
    WD_SAMPLER,
    WD_UNIFORM_BUFFER,
    WD_PUSH_CONSTANT, // only supported by Vulkan backend
    WD_SUBPASS_INPUT, // only supported by Vulkan
    WD_STAGE_INPUT, // used for ins 
    WD_STAGE_OUTPUT, // used for outs 
    WD_STORAGE_BUFFER, 
};

class ShaderParameter {
    ShaderParameterType paramType;
    uint8_t decorationSet;
    uint8_t decorationBinding;
};

class Shader {
    public:
    private:
        std::vector<uint8_t> shaderByteCode;
        std::vector<ShaderParameter> uniforms;
        std::vector<ShaderParameter> inputs;
        std::vector<ShaderParameter> outputs;
};

}
#endif