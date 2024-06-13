#ifndef WADO_RENDERGRAPH_H
#define WADO_RENDERGRAPH_H

#include "WdCommandList.h"
#include "RenderGraphShaderTypes.h"
#include "WdBuffer.h"
#include "WdImage.h"
#include "WdLayer.h"
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <iostream>

namespace Wado::RenderGraph {

    using WdRDGExecuteCallbackFn = std::function<void(Wado::GAL::WdCommandList& cmdList)>;

    class WdRenderGraphBuilder {
        public:
            WdRenderGraphBuilder(Wado::GAL::WdLayer* layer) : _layer(layer) { };
            ~WdRenderGraphBuilder();

            // templated by param type 
            template <typename T>
            void addRenderPass(const std::string& passName, const T& params, WdRDGExecuteCallbackFn callback);
            RDBufferHandle createBuffer(const GAL::WdBufferDescription& description);
            RDTextureHandle createTexture(const GAL::WdImageDescription& description);

            RDTextureHandle registerExternalTexture(const GAL::WdImage* image);
            RDBufferHandle registerExternalBuffer(const GAL::WdBuffer* buffer);

            void execute();
        private:

            void cullUnused();
            void compile();

            using RDRenderPassID = size_t;

            RDRenderPassID CUMMULATIVE_RENDERPASS_ID = 1;
            static const RDRenderPassID UNDEFINED_RENDERPASS_ID = 0;

            using RDRenderPass = struct RDRenderPass {
                RDRenderPass(WdRDGExecuteCallbackFn _executeFunction) {
                    executeFunction = _executeFunction;
                 };

                WdRDGExecuteCallbackFn executeFunction;
            };

            using RDReadWriteSet = struct RDReadWriteSet {
                RDRenderPassID writerID;
                std::unordered_set<RDRenderPassID> readerIDs;
            };

            using RDResourceInfo = struct RDResourceInfo {
                RDResourceInfo(RDRenderPassID _firstUse, RDRenderPassID _lastUse, uint64_t _version) : firstUse(_firstUse), lastUse(_lastUse), version(_version) { };

                RDRenderPassID firstUse;
                RDRenderPassID lastUse;
                uint64_t version;
            };

            using RDResourceTransition = struct RDResourceTransition {
                RDTextureHandle texture;
                uint64_t layout1;
                uint64_t layout2;
            };

            void registerTransition(const RDResourceTransition& transition, Wado::GAL::WdCommandList& cmdList);

            using RDSubmitGroup = struct RDSubmitGroup {
                std::vector<RDRenderPass> renderPasses;
                std::vector<RDResourceTransition> resourceTransitions;
            };

            using RDGraphNode = struct RDGraphNode {
                uint64_t underlyingResourceID;
                uint64_t dependencyLevel;
                uint64_t version;
                uint64_t producerCount;
                uint64_t consumerCount;
                std::unordered_set<RDGraphNode *> producers;
                std::unordered_set<RDGraphNode *> consumers;
            };

            std::unordered_map<uint64_t, RDGraphNode*> _resourceRDGNodes;
            std::unordered_map<RDRenderPassID, RDGraphNode*> _renderPassRDGNodes;

            using RDDependencyLevel = struct RDDependencyLevel {
                std::vector<RDGraphNode *> passes;
                std::vector<RDGraphNode *> resourceTransitions;
            };

            std::vector<RDDependencyLevel> _sortedGraph;

            std::vector<RDRenderPass> _renderPasses;
            std::vector<RDGraphNode *> _noReadNodes;
            std::vector<RDResourceInfo> _textureRegistry;
            std::vector<RDResourceInfo> _bufferRegistry;

            Wado::GAL::WdLayer* _layer;

            inline RDGraphNode *makeRenderPassNode(uint64_t producerCount, uint64_t consumerCount) {
                RDGraphNode *renderPassNode = new RDGraphNode();
                renderPassNode->consumerCount = consumerCount;
                renderPassNode->producerCount = producerCount;
                renderPassNode->underlyingResourceID = CUMMULATIVE_RENDERPASS_ID;
                renderPassNode->dependencyLevel = 0;

                return renderPassNode;
            };


            inline void addResourceRead(const RDResourceHandle resourceHandle, RDGraphNode *renderPassNode) {
                RDResourceInfo& resInfo = _textureRegistry.at(resourceHandle);
                if (resInfo.firstUse == UNDEFINED_RENDERPASS_ID) {
                    resInfo.firstUse = CUMMULATIVE_RENDERPASS_ID;

                    RDGraphNode *resInfoNode  = new RDGraphNode();
                    resInfoNode->underlyingResourceID = resourceHandle;
                    resInfoNode->version = 0;
                    _resourceRDGNodes[resourceHandle] = resInfoNode;
                };

                resInfo.lastUse = CUMMULATIVE_RENDERPASS_ID;
                uint64_t versionedHandle = resourceHandle | (resInfo.version << 32);
                //std::cout << "Original handle: " << (void *) resourceHandle << std::endl;
                //std::cout << "Versioned handle: " << (void *) versionedHandle << std::endl;
                RDGraphNode *resInfoNode  = _resourceRDGNodes.at(versionedHandle);

                resInfoNode->consumerCount++;
                renderPassNode->producers.insert(resInfoNode);
                resInfoNode->consumers.insert(renderPassNode);
            };

            inline void addResourceWrite(const RDResourceHandle resourceHandle, RDGraphNode *renderPassNode) {
                RDResourceInfo& resInfo = _textureRegistry.at(resourceHandle);
                if (resInfo.firstUse == UNDEFINED_RENDERPASS_ID) {
                    resInfo.firstUse = CUMMULATIVE_RENDERPASS_ID;

                    RDGraphNode *resInfoNode  = new RDGraphNode();
                    resInfoNode->version = 0;
                    resInfoNode->underlyingResourceID = resourceHandle;
                    _resourceRDGNodes[resourceHandle] = resInfoNode;
                };

                resInfo.lastUse = CUMMULATIVE_RENDERPASS_ID;
                // Need to be a consumer of the last version 

                RDGraphNode *prevResInfoNode = _resourceRDGNodes.at(resourceHandle | (resInfo.version << 32));

                prevResInfoNode->consumerCount++;
                prevResInfoNode->consumers.insert(renderPassNode);
                renderPassNode->producers.insert(prevResInfoNode);

                resInfo.version++;

                RDGraphNode *resInfoNode  = new RDGraphNode();
                resInfoNode->underlyingResourceID = resourceHandle;
                resInfoNode->producerCount++;
                resInfoNode->version = resInfo.version;
                resInfoNode->producers.insert(renderPassNode);
                uint64_t versionedHandle = resourceHandle | (resInfo.version << 32);
                //std::cout << "Original handle: " << (void *) resourceHandle << std::endl;
                //std::cout << "Versioned handle: " << (void *) versionedHandle << std::endl;
                // Insert the new node now 
                _resourceRDGNodes[versionedHandle] = resInfoNode;

                renderPassNode->consumers.insert(resInfoNode);
            };
    };
};

#endif