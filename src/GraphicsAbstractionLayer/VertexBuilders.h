#ifndef H_WADO_VERTEX_BUILDERS
#define H_WADO_VERTEX_BUILDERS

#include "GraphicsAbstractionLayer.h"
#include <glm/glm.hpp>

namespace Wado::GAL {

    using GBufferVertex = struct GBufferVertex {
        alignas(16) glm::vec3 pos;
        alignas(16) glm::vec3 color;
        alignas(16) glm::vec3 normal;
        alignas(8)  glm::vec2 texCoord;
        alignas(4)  uint32_t texIndex; 
    };

    // Vertex layout is:
    // vec3 pos
    // vec3 color
    // vec3 normal
    // vec2 textureCoords
    // uint32_t textureIndex
    class GBufferVertexBuilder : public WdVertexBuilder {
        public:
            GBufferVertexBuilder();
            std::vector<WdVertexBinding> getBindingDescriptions();
            WdInputTopology getInputTopology();
        private:
            std::vector<WdVertexBinding> _bindings;
            WdInputTopology _topology;
    };
};

#endif