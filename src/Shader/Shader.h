#ifndef H_WD_SHADER
#define H_WD_SHADER

#include "GraphicsAbstractionLayer.h"
#include <cstdint>
#include <vector>

namespace Wado::Shader {

using ShaderByteCode = std::vector<uint8_t>;

class Shader {
    public:
        virtual std::vector<ShaderParameter> getShaderUniforms() = 0;
    private:
        ShaderByteCode shaderByteCode;
        std::vector<ShaderParameter> uniforms;
        std::vector<ShaderParameter> inputs;
        std::vector<ShaderParameter> outputs;
};

class VertexShader : public Shader {
    public:
        virtual std::vector<ShaderParameter> getShaderUniforms() = 0;
    private:
        ShaderByteCode shaderByteCode;
        std::vector<ShaderParameter> uniforms;
        std::vector<ShaderParameter> inputs;
        std::vector<ShaderParameter> outputs;
        std::vector<ShaderParameter> subpassInputs; // uniforms under any other API and Vulkan
        // but they need to be handled separately in Vulkan. Since these don't really 
        // exist for other APIs I thought it was fine to specialize it instead of 
        // removing it 
}

}
#endif