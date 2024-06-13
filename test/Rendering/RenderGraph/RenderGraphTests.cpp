#include <gtest/gtest.h>

#include "DebugLayer.h"
#include "RenderGraphShaderTypes.h"
#include "RenderGraph.h"

#include "RenderGraphTest.h"

TEST(RenderGraphTest, CorrectTopologicalSort) {
    using namespace Wado::RenderGraph;

    Wado::GAL::Debug::DebugLayer layer = Wado::GAL::Debug::DebugLayer();
    WdRenderGraphBuilder rdgBuilder = WdRenderGraphBuilder(&layer);

    RDTextureHandle diffuseTexture = rdgBuilder.registerExternalTexture(nullptr);
    ASSERT_EQ(diffuseTexture, 0);
    RDTextureHandle normalTexture = rdgBuilder.registerExternalTexture(nullptr);
    ASSERT_EQ(normalTexture, 1);
    RDTextureHandle metallicTexture = rdgBuilder.registerExternalTexture(nullptr);
    ASSERT_EQ(metallicTexture, 2);
    RDTextureHandle roughnessTexture = rdgBuilder.registerExternalTexture(nullptr);
    ASSERT_EQ(roughnessTexture, 3);
    RDTextureHandle aoTexture = rdgBuilder.registerExternalTexture(nullptr);
    ASSERT_EQ(aoTexture, 4);
    
    Wado::GAL::WdFormat defaultFormat = Wado::GAL::WdFormat::WD_FORMAT_R8G8B8_SRGB;
    Wado::GAL::WdExtent2D defaultExtent;
    defaultExtent.height = 0;
    defaultExtent.width = 0;

    Wado::GAL::WdClearValue clearValue;
    clearValue.color = Wado::GAL::DEFAULT_COLOR_CLEAR;

    Wado::GAL::WdImageUsageFlags flags = Wado::GAL::WdImageUsage::WD_COLOR_ATTACHMENT;
    Wado::GAL::WdSampleCount sampleCount = Wado::GAL::WdSampleCount::WD_SAMPLE_COUNT_1;
    Wado::GAL::WdMipCount mipCount = 0;

    Wado::GAL::WdImageDescription desc(defaultFormat, flags, defaultExtent, clearValue, mipCount, sampleCount);

    RDTextureHandle diffuseProperties = rdgBuilder.createTexture(desc);
    ASSERT_EQ(diffuseProperties, 5);
    RDTextureHandle specularProperties = rdgBuilder.createTexture(desc);
    ASSERT_EQ(specularProperties, 6);
    RDTextureHandle meshProperties = rdgBuilder.createTexture(desc);
    ASSERT_EQ(meshProperties, 7);
    RDTextureHandle positionProperties = rdgBuilder.createTexture(desc);
    ASSERT_EQ(positionProperties, 8);

    GBufferFragmentParams gBufferParams;
    gBufferParams.diffuseProperties = diffuseProperties;
    gBufferParams.specularProperties = specularProperties;
    gBufferParams.meshProperties = meshProperties;
    gBufferParams.positionProperties = positionProperties;

    gBufferParams.diffuseSampler.texture = diffuseTexture;
    gBufferParams.normalMapSampler.texture = normalTexture;
    gBufferParams.metallicMapSampler.texture = metallicTexture;
    gBufferParams.roughnessMapSampler.texture = roughnessTexture;
    gBufferParams.aoMapSampler.texture = aoTexture;

    TestShaderParams testParams;
    testParams.Test1 = diffuseProperties;

    TestShaderParams2 testParams2;
    testParams2.Test1 = diffuseProperties;
    testParams2.Test2 = specularProperties;
    
    Wado::RenderGraph::WdRDGExecuteCallbackFn dummyFunc = [](Wado::GAL::WdCommandList& cmdList) { std::cout << "aa" << std::endl; };
    std::string name = "abc";
    rdgBuilder.addRenderPass(name, gBufferParams, dummyFunc);
    rdgBuilder.addRenderPass(name, testParams, dummyFunc);
    rdgBuilder.addRenderPass(name, testParams2, dummyFunc);

    rdgBuilder.execute();
};