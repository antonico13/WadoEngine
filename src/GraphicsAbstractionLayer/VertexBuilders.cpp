#include "GraphicsAbstractionLayer.h"
#include "VertexBuilders.h"

namespace Wado::GAL {

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