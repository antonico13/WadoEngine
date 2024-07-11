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
            
        private:
    };

    class WdVertexShader : virtual WdShader {
        public:

        private:
    };

    class WdFragmentShader : virtual WdShader {
        public:
            
        private:
    };
};

#endif