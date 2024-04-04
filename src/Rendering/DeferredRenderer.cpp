#include "DeferredRenderer.h"
#include "GraphicsAbstractionLayer.h"
#include "VertexBuilders.h"
#include "Shader.h"

#include <vector>

class Scene {

};

namespace Wado::Rendering {

void DeferredRenderer::init() {

};


void DeferredRenderer::createDepthAttachment() {
    WdFormat depthFormat = _graphicsLayer->findSupportedHardwareFormat(
        {GAL::WdFormat::WD_FORMAT_D32_SFLOAT, GAL::WdFormat::WD_FORMAT_D32_SFLOAT_S8_UINT, GAL::WdFormat::WD_FORMAT_D24_UNORM_S8_UINT}, 
        GAL::WdImageTiling::WD_IMAGE_TILING_OPTIMAL, 
        GAL::WdFormatFeatures::WD_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT);

    depthAttachment = _graphicsLayer->create2DImage(swapchainExtent, 1, GAL::WdSampleCount::WD_SAMPLE_COUNT_1, depthFormat, GAL::WdImageUsage::WD_DEPTH_STENCIL_ATTACHMENT); 
    
    _graphicsLayer->prepareImageFor(depthImage, GAL::WdImageUsage::WD_UNDEFINED, GAL::WdImageUsage::WD_DEPTH_STENCIL_ATTACHMENT);
};

void DeferredRenderer::createDeferredColorAttachments(GAL::WdFormat attachmentFormat) {
    deferredColorAttachments.resize(DEFERRED_ATTACHMENT_COUNT);
    // diffuse image, specular image, mesh image, position image 
    for (int i = 0; i < DEFERRED_ATTACHMENT_COUNT; i++) {
        GAL::WdImage attachment = _graphicsLayer->create2DImage(swapchainExtent, 1, GAL::WdSampleCount::WD_SAMPLE_COUNT_1, attachmentFormat, GAL::WdImageUsage::WD_COLOR_ATTACHMENT | GAL::WdImageUsage::WD_INPUT_ATTACHMENT);
        deferredColorAttachments.push_back(attachment);
    }
};


// draws one frame, maybe needs a better name 

void DeferredRender::render(Scene scene) {
    
    // these should be in init for the first time, otherwise cache

    // Extract all shaders from scene 
    Shader::Shader gbufferVertexShader;
    Shader::Shader gbufferFragmentShader;
    Shader::Shader deferredVertexShader;
    Shader::Shader deferredFragmentShader;
    
    // Create vertex builder here from Vertex properties

    std::shared_ptr<GAL::WdVertexBuilder> gBufferBuilder = GAL::VertexBuilderManager::getManager()->getBuilder<GBufferVertexBuilder>();
    std::shared_ptr<GAL::WdVertexBuilder> deferredBuilder = GAL::VertexBuilderManager::getManager()->getBuilder<DeferredVertexBuilder>();


    GAL::WdViewportProperties viewportProperties{};
    viewportProperties.startCoords = {0, 0};
    viewportProperties.endCoords = {WIDTH, HEIGHT};
    viewportProperties.scissor = {{0, 0}, {WIDTH, HEIGHT}};
    
    // these should be friend classes as well 
    // need to say here to use depth... should prob be created in the underlying GAL 
    GAL::WdPipeline gBufferPipeline = _graphicsLayer->createPipeline(gbufferVertexShader, gbufferFragmentShader, gBufferBuilder.get(), viewportProperties);

    GAL::WdPipeline deferredPipeline = _graphicsLayer->createPipeline(deferredVertexShader, deferredFragmentShader, deferredBuilder.get(), viewportProperties);

    // ^^ this could be/should be cached somehow so we actually only look for these pipelines if they don't exist.

    std::vector<GAL::WdPipeline> pipelines = {gBufferPipeline, deferredPipeline};

    GAL::WdRenderPass deferredRenderPass = _graphicsLayer->createRenderPass(pipelines);

    createDeferredColorAttachments();
};  

};