#include "RenderGraph.h"

#include <iostream>

namespace Wado::RenderGraph {

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
        // Pessimistic size 
        std::vector<std::vector<RDGraphNode *>> sortedGraph((_renderPassRDGNodes.size() + _resourceRDGNodes.size()));

        size_t levels = 0;

        // Do topological sort.
        while (!_noReadNodes.empty()) {
            // Pop front node 
            RDGraphNode *frontNode = *_noReadNodes.begin();
            _noReadNodes.erase(_noReadNodes.begin());
            // Add to sorted graph
            sortedGraph[frontNode->dependencyLevel].emplace_back(frontNode);

            for (RDGraphNode *consumerNode : frontNode->consumers) {
                // Decrement producer count
                //std::cout << "Node " << frontNode->underlyingResourceID << " version " << frontNode->version << " has consumer: " << consumerNode->underlyingResourceID << std::endl;
                consumerNode->producerCount--;
                if (consumerNode->producerCount == 0) {
                    //std::cout << "Producer count for that node reached 0 " << std::endl;
                    // can add to no read set now
                    consumerNode->dependencyLevel = frontNode->dependencyLevel + 1;
                    if (consumerNode->dependencyLevel > levels) {
                        levels = consumerNode->dependencyLevel;
                    };

                    _noReadNodes.emplace_back(consumerNode);
                };
            };
        };
        std::cout << levels << std::endl;
        sortedGraph.resize(levels + 1);
        _sortedGraph.resize((levels >> 1) + 1);
        for (int i = 0; i < sortedGraph.size(); i = i + 2) {
            //std::cout << "Depedency level: " << (i >> 1) << std::endl; 
            _sortedGraph[i >> 1].passes = sortedGraph[i];
            _sortedGraph[i >> 1].resourceTransitions = sortedGraph[i + 1];
            //std::cout << "Render passes" << std::endl;
            // for (RDGraphNode *sortedNode : sortedGraph[i]) {
            //     std::cout << "Underlying ID: " << sortedNode->underlyingResourceID << " Version: " << sortedNode->version << std::endl;
            // }; 
            // std::cout << "Resource transitions: " << std::endl;
            // for (RDGraphNode *sortedNode : sortedGraph[i + 1]) {
            //     std::cout << "Underlying ID: " << sortedNode->underlyingResourceID << " Version: " << sortedNode->version << std::endl;
            // }; 
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

    void WdRenderGraphBuilder::registerTransition(const RDResourceTransition& transition, Wado::GAL::WdCommandList& cmdList) {
        // TODO, don't have this feature in the CMD list 
        std::cout << "Transition: " << transition.texture << " from " << transition.layout1 << " to " << transition.layout2 << std::endl;
    };

    // this should be build not execute 
    void WdRenderGraphBuilder::execute() {
        //cullUnused();
        compile();
        Wado::GAL::WdCommandList &cmdList = _layer->createCommandList().operator*();
        // We have the topologically sorted render graph now, even vectors are passes, odd vectors are resource dependencies
        for (const RDDependencyLevel& depLevel : _sortedGraph) {
            for (const RDGraphNode *renderPassNode : depLevel.passes) {
                std::cout << "Render pass id: " << renderPassNode->underlyingResourceID;
                _renderPasses.at(renderPassNode->underlyingResourceID - 1).executeFunction(cmdList);
            };
            for (const RDGraphNode *resourceNode : depLevel.resourceTransitions) {
                RDResourceTransition transition;
                transition.layout1 = 0;
                transition.layout2 = 0;
                transition.texture = resourceNode->underlyingResourceID;
                registerTransition(transition, cmdList);
            };
        };
    };

    void WdRenderGraphBuilder::cullUnused() {
        std::vector<RDGraphNode *> unconsumedResources;

        for (std::unordered_map<uint64_t, RDGraphNode*>::iterator it = _resourceRDGNodes.begin(); it != _resourceRDGNodes.end(); ++it) {
            if (it->second->consumerCount == 0) {
                //std::cout << "Found unconsumed resource: " << it->second->underlyingResourceID << ", version: " << it->second->version << std::endl;
                unconsumedResources.emplace_back(it->second);
            };
        };
        
        // Perform flood-fill to remove nodes from graph 
        while (!unconsumedResources.empty()) {  
            // We know all are resources so they have one producer 
            RDGraphNode *resNode = *unconsumedResources.begin();
            unconsumedResources.erase(unconsumedResources.begin());
            // TODO: how to mark final render pass as that it always has a consumer for its resources? 
            // This deletes every graph otherwise 
            RDGraphNode *producerNode = *resNode->producers.begin();
            producerNode->consumerCount--;
            if (producerNode->consumerCount == 0) {
                //std::cout << "Found render pass that can be erased: " << producerNode->underlyingResourceID << std::endl;
                for (RDGraphNode *renderPassProducer : producerNode->producers) {
                    renderPassProducer->consumerCount--;
                    if (renderPassProducer->consumerCount == 0) {
                        unconsumedResources.emplace_back(renderPassProducer);
                    };
                };
            };
        };
    };
};
