#ifndef WADO_RDG_TEST_H
#define WADO_RDG_TEST_H

#include "RenderGraphShaderTypes.h"
#include "RenderGraph.h"

namespace Wado::RenderGraph {
    using GBufferFragmentParams = struct GBufferFragmentParams {
        static const size_t MAX_TEXTURE_SETS = 32;

        RDRenderTarget diffuseProperties;
        RDRenderTarget specularProperties;
        RDRenderTarget meshProperties;
        RDRenderTarget positionProperties;

        RDCombinedTextureSampler diffuseSampler[MAX_TEXTURE_SETS];
        RDCombinedTextureSampler normalMapSampler[MAX_TEXTURE_SETS];
        RDCombinedTextureSampler metallicMapSampler[MAX_TEXTURE_SETS];
        RDCombinedTextureSampler roughnessMapSampler[MAX_TEXTURE_SETS];
        RDCombinedTextureSampler aoMapSampler[MAX_TEXTURE_SETS];
    };


    template <>
    void WdRenderGraphBuilder::addRenderPass(const std::string& passName, const GBufferFragmentParams& params, WdRDGExecuteCallbackFn&& callback) {

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