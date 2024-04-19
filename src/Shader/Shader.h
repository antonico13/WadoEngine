#ifndef WADO_SHADER
#define WADO_SHADER

#include <cstdint>
#include <vector>

namespace Wado::Shader {

    using WdShaderByteCode = const std::vector<uint8_t>&;

    class WdShader {
        public:
        private:
            WdShaderByteCode shaderByteCode;
    };
}
#endif