#ifndef H_WADO_VERTEX_BUILDERS
#define H_WADO_VERTEX_BUILDERS

#include "GraphicsAbstractionLayer.h"
#include <glm/glm.hpp>

#include <map>
#include <string>
#include <memory>

#define CREATE_VERTEX_BUILDER(builderName) (std::make_shared<builderName>())

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
            friend class VertexBuilderManager;
            std::vector<WdVertexBinding> getBindingDescriptions();
            WdInputTopology getInputTopology();
        private:
            GBufferVertexBuilder();
            std::vector<WdVertexBinding> _bindings;
            WdInputTopology _topology;
    };

    class VertexBuilderManager {
        public:
            static std::shared_ptr<VertexBuilderManager> getManager();
            std::shared_ptr<WdVertexBuilder> getBuilder(std::string builderName);
        private:
            VertexBuilderManager() {};
            static std::shared_ptr<VertexBuilderManager> manager;
            std::map<std::string, std::shared_ptr<WdVertexBuilder>> vertexBuilders;
    };
};

#endif