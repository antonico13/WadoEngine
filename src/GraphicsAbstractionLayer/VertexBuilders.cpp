#include "GraphicsAbstractionLayer.h"
#include "VertexBuilders.h"

#include <memory>

namespace Wado::GAL {

    VertexBuilderManager::manager;

    template<GBufferVertexBuilder>
    std::string getBuilderName() {
        return "GBufferVertexBuilder";
    };

    template<DeferredVertexBuilder>
    std::string getBuilderName() {
        return "DeferredVertexBuilder";
    };

    std::shared_ptr<VertexBuilderManager> VertexBuilderManager::getManager() {
        if (manager == nullptr) {
            manager.reset(new VertexBuilderManager());
        }
        return manager;
    };

    template <class T> 
    std::shared_ptr<WdVertexBuilder> VertexBuilderManager::getBuilder() {
        std::string builderName = getBuilderName<T>();
        std::map<std::string, std::shared_ptr<WdVertexBuilder>>::iterator it = vertexBuilders.find(builderName);
        
        if (it != vertexBuilders.end()) {
            return *it;
        };

        std::shared_ptr<WdVertexBuilder> builder = static_cast<std::shared_ptr<WdVertexBuilder>>(std::make_shared<T>());
        
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

    std::vector<WdVertexBinding> DeferredVertexBuilder::getBindingDescriptions() {
        return _bindings; // will be empty array 
    };

    WdInputTopology GBufferVertexBuilder::getInputTopology() {
        return WD_TOPOLOGY_TRIANGLE_LIST; // doesn't actually matter? maybe I should make it "NONE" ?
    };
};