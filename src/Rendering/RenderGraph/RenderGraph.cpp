#include "RenderGraph.h"

#include <iostream>

namespace Wado::RenderGraph {

    WdRenderGraphBuilder::WdRenderGraphBuilder() {

    };

    WdRenderGraphBuilder::~WdRenderGraphBuilder() {
        for (std::unordered_map<uint64_t, RDGraphNode*>::iterator it = _resourceRDGNodes.begin(); it != _resourceRDGNodes.end(); ++it) {
            free(it->second);
        };

        for (std::unordered_map<RDRenderPassID, RDGraphNode*>::iterator it = _renderPassRDGNodes.begin(); it != _renderPassRDGNodes.end(); ++it) {
            free(it->second);
        };
    };

    void WdRenderGraphBuilder::compile() {
        // While there are render passes with 0 reads, do this
        std::vector<RDGraphNode *> sortedGraph;
        
        std::cout << "Render passes look like: " << std::endl;

        for (std::unordered_map<RDRenderPassID, RDGraphNode*>::iterator it = _renderPassRDGNodes.begin(); it != _renderPassRDGNodes.end(); ++it) {
            std::cout << "Render pass: " << it->second->underlyingResourceID << " has: " << std::endl;
            std::cout << "Producers: " << std::endl;
            for (RDGraphNode * producer : it->second->producers) {
                std::cout << "Render resource " << producer->underlyingResourceID << ", version: " << producer->version << std::endl;
            };
            std::cout << "Consumers: " << std::endl;
            for (RDGraphNode * consumer : it->second->consumers) {
                std::cout << "Render resource " << consumer->underlyingResourceID << ", version: " << consumer->version << std::endl;
            };
        };

        for (std::unordered_map<uint64_t, RDGraphNode*>::iterator it = _resourceRDGNodes.begin(); it != _resourceRDGNodes.end(); ++it) {
            std::cout << "Resource: " << it->second->underlyingResourceID << ", version: " << it->second->version << " has: " << std::endl;
            std::cout << "Producers: " << std::endl;
            for (RDGraphNode * producer : it->second->producers) {
                std::cout << "Render pass " << producer->underlyingResourceID << ", version: " << producer->version << std::endl;
            };
            std::cout << "Consumers: " << std::endl;
            for (RDGraphNode * consumer : it->second->consumers) {
                std::cout << "Render pass " << consumer->underlyingResourceID << ", version: " << consumer->version << std::endl;
            };        
        };

        // Do topological sort.
        while (!_noReadNodes.empty()) {
            // Pop front node 
            RDGraphNode *frontNode = *_noReadNodes.begin();
            _noReadNodes.erase(_noReadNodes.begin());
            // Add to sorted graph
            sortedGraph.emplace_back(frontNode);

            for (RDGraphNode *consumerNode : frontNode->consumers) {
                // Decrement producer count
                std::cout << "Node " << frontNode->underlyingResourceID << " version " << frontNode->version << " has consumer: " << consumerNode->underlyingResourceID << std::endl;
                consumerNode->producerCount--;
                if (consumerNode->producerCount == 0) {
                    std::cout << "Producer count for that node reached 0 " << std::endl;
                    // can add to no read set now
                    _noReadNodes.emplace_back(consumerNode);
                };
            };
        };

        for (RDGraphNode *sortedNode : sortedGraph) {
            std::cout << "Underlying ID: " << sortedNode->underlyingResourceID << " Version: " << sortedNode->version << std::endl;
        };
    };

    RDBufferHandle WdRenderGraphBuilder::createBuffer(const GAL::WdBufferDescription& description) {
        RDTextureHandle handle = _bufferRegistry.size();
        _bufferRegistry.emplace_back(0, 0, 0);
        return handle;
    };

    RDTextureHandle WdRenderGraphBuilder::createTexture(const GAL::WdImageDescription& description) {
        RDTextureHandle handle = _textureRegistry.size();
        _textureRegistry.emplace_back(0, 0, 0);
        return handle;
    };

    RDTextureHandle WdRenderGraphBuilder::registerExternalTexture(const GAL::WdImage* image) {
        RDTextureHandle handle = _textureRegistry.size();
        _textureRegistry.emplace_back(0, 0, 0);
        return handle;
    }; 
    
    RDBufferHandle WdRenderGraphBuilder::registerExternalBuffer(const GAL::WdBuffer* buffer) {
        RDTextureHandle handle = _bufferRegistry.size();
        _bufferRegistry.emplace_back(0, 0, 0);
        return handle;
    };

    void WdRenderGraphBuilder::execute() {
        compile();
    };
};
