#ifndef WADO_RDG_TEST_H
#define WADO_RDG_TEST_H

#include "RenderGraphShaderTypes.h"
#include "RenderGraph.h"

namespace Wado::RenderGraph {
    using GBufferFragmentParams = struct GBufferFragmentParams {
//        static const size_t MAX_TEXTURE_SETS = 32;
        RDRenderTarget diffuseProperties;
        RDRenderTarget specularProperties;
        RDRenderTarget meshProperties;
        RDRenderTarget positionProperties;

        RDCombinedTextureSampler diffuseSampler;//[MAX_TEXTURE_SETS];
        RDCombinedTextureSampler normalMapSampler;//[MAX_TEXTURE_SETS];
        RDCombinedTextureSampler metallicMapSampler;//[MAX_TEXTURE_SETS];
        RDCombinedTextureSampler roughnessMapSampler;//[MAX_TEXTURE_SETS];
        RDCombinedTextureSampler aoMapSampler;//[MAX_TEXTURE_SETS];
    };


    template <>
    void WdRenderGraphBuilder::addRenderPass(const std::string& passName, const GBufferFragmentParams& params, WdRDGExecuteCallbackFn&& callback) {
        RDGraphNode *renderPassNode = makeRenderPassNode(9, 4);

        addResourceRead(params.diffuseSampler.texture, renderPassNode);
        addResourceRead(params.normalMapSampler.texture, renderPassNode);
        addResourceRead(params.metallicMapSampler.texture, renderPassNode);
        addResourceRead(params.roughnessMapSampler.texture, renderPassNode);
        addResourceRead(params.aoMapSampler.texture, renderPassNode);

        addResourceWrite(params.diffuseProperties, renderPassNode);
        addResourceWrite(params.specularProperties, renderPassNode);
        addResourceWrite(params.meshProperties, renderPassNode);
        addResourceWrite(params.positionProperties, renderPassNode);

        _renderPassRDGNodes.emplace(CUMMULATIVE_RENDERPASS_ID, renderPassNode);
        _noReadNodes.emplace_back(renderPassNode);
        CUMMULATIVE_RENDERPASS_ID++;
    };

    using TestShaderParams = struct TestShaderParams {
        RDRenderTarget Test1;
    };


    template <>
    void WdRenderGraphBuilder::addRenderPass(const std::string& passName, const TestShaderParams& params, WdRDGExecuteCallbackFn&& callback) {
        RDGraphNode *renderPassNode = makeRenderPassNode(1, 1);

        addResourceWrite(params.Test1, renderPassNode);

        _renderPassRDGNodes.emplace(CUMMULATIVE_RENDERPASS_ID, renderPassNode);
        CUMMULATIVE_RENDERPASS_ID++;
    };

    using TestShaderParams2 = struct TestShaderParams2 {

        RDRenderTarget Test1;
        RDRenderTarget Test2;
    };


    template <>
    void WdRenderGraphBuilder::addRenderPass(const std::string& passName, const TestShaderParams2& params, WdRDGExecuteCallbackFn&& callback) {
        RDGraphNode *renderPassNode = makeRenderPassNode(2, 2);

        addResourceWrite(params.Test1, renderPassNode);
        addResourceWrite(params.Test2, renderPassNode);

        _renderPassRDGNodes.emplace(CUMMULATIVE_RENDERPASS_ID, renderPassNode);
        CUMMULATIVE_RENDERPASS_ID++;
    };

    using DeferredLightingShaderParams = struct DeferredLightingShaderParams {
        RDRenderTarget outColor;

        RDSubpassInput diffuseInput;
        RDSubpassInput specularInput;
        RDSubpassInput meshInput;
        RDSubpassInput positionInput;

        RDBufferHandle pointLight;
        RDBufferHandle cameraProperties;
    };

    template <>
    void WdRenderGraphBuilder::addRenderPass(const std::string& passName, const DeferredLightingShaderParams& params, WdRDGExecuteCallbackFn&& callback) {

    };

};
#endif