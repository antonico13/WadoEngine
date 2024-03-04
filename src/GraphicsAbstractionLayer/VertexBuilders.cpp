#include "GraphicsAbstractionLayer.h"
#include "VertexBuilders.h"

#include <memory>

namespace Wado::GAL {

    VertexBuilderManager::manager;

    std::shared_ptr<VertexBuilderManager> VertexBuilderManager::getManager() {
        if (manager == nullptr) {
            manager.reset(new VertexBuilderManager());
        }
        return manager;
    };

    std::shared_ptr<WdVertexBuilder> VertexBuilderManager::getBuilder(std::string builderName) {
        std::map<std::string, std::shared_ptr<WdVertexBuilder>>::iterator it = vertexBuilders.find(builderName);
        
        if (it != vertexBuilders.end()) {
            return *it;
        };

        std::shared_ptr<WdVertexBuilder> builder = static_cast<std::shared_ptr<WdVertexBuilder>>(CREATE_VERTEX_BUILDER(builderName));
        
        vertexBuilders.emplace(builderName, builder);
        
        return builder;
    };


    GBufferVertexBuilder::GBufferVertexBuilder() {
        _bindings.resize(1);

        _bindings[0].rate = WD_VERTEX_RATE_VERTEX;
        _bindings[0].stride = sizeof(GBufferVertex);
        
        // locations are in order
        std::vector<WdVertexAttribute> vertexAttributes(5);
        vertexAttributes[0].format = WD_FORMAT_R32G32B32_SFLOAT; 
        vertexAttributes[0].offset = offsetof(GBufferVertex, pos);

        vertexAttributes[1].format = WD_FORMAT_R32G32B32_SFLOAT; 
        vertexAttributes[1].offset = offsetof(GBufferVertex, color);

        vertexAttributes[2].format = WD_FORMAT_R32G32B32_SFLOAT; 
        vertexAttributes[2].offset = offsetof(GBufferVertex, normal);

        vertexAttributes[3].format = WD_FORMAT_R32G32_SFLOAT; 
        vertexAttributes[3].offset = offsetof(GBufferVertex, texCoord);

        vertexAttributes[4].format = WD_FORMAT_R32_SINT; 
        vertexAttributes[4].offset = offsetof(GBufferVertex, texIndex);

        _bindings[0].attributeDescriptions = vertexAttributes;

        _topology = WD_TOPOLOGY_TRIANGLE_LIST;
    };

    std::vector<WdVertexBinding> GBufferVertexBuilder::getBindingDescriptions() {
        return _bindings;
    };

    WdInputTopology GBufferVertexBuilder::getInputTopology() {
        return _topology;
    };
};