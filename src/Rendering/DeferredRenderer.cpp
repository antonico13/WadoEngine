#include "DeferredRenderer.h"
#include "GraphicsAbstractionLayer.h"
#include "VertexBuilders.h"
#include "Shader.h"

#include <vector>

class Scene {

};

namespace Wado::Rendering {

void DeferredRender::init() {

};


void DeferredRender::createDepthAttachment() {
    WdFormat depthFormat = _graphicsLayer->findSupportedHardwareFormat(
        {GAL::WdFormat::WD_FORMAT_D32_SFLOAT, GAL::WdFormat::WD_FORMAT_D32_SFLOAT_S8_UINT, GAL::WdFormat::WD_FORMAT_D24_UNORM_S8_UINT}, 
        GAL::WdImageTiling::WD_IMAGE_TILING_OPTIMAL, 
        GAL::WdFormatFeatures::WD_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT);

    depthAttachment = _graphicsLayer->create2DImage(swapchainExtent, 1, GAL::WdSampleCount::WD_SAMPLE_COUNT_1, depthFormat, GAL::WdImageUsage::WD_DEPTH_STENCIL_ATTACHMENT); 
    
    // set clear value manually 

    GAL::WdDepthStencilValue depthClear = {1.0f, 0};
    depthAttachment.clearValue.depthStencil = depthClear;

    _graphicsLayer->prepareImageFor(depthImage, GAL::WdImageUsage::WD_UNDEFINED, GAL::WdImageUsage::WD_DEPTH_STENCIL_ATTACHMENT);
};

void DeferredRender::createDeferredColorAttachments(GAL::WdFormat attachmentFormat) {
    deferredColorAttachments.resize(DEFERRED_ATTACHMENT_COUNT);
    // diffuse image, specular image, mesh image, position image 
    GAL::WdColorValue colorClear = {0.0f, 0.0f, 0.0f, 1.0f};
    for (int i = 0; i < DEFERRED_ATTACHMENT_COUNT; i++) {
        GAL::WdImage attachment = _graphicsLayer->create2DImage(swapchainExtent, 1, GAL::WdSampleCount::WD_SAMPLE_COUNT_1, attachmentFormat, GAL::WdImageUsage::WD_COLOR_ATTACHMENT | GAL::WdImageUsage::WD_INPUT_ATTACHMENT);
        // need to set clear values here manually
        attachment.clearValue.color = colorClear;
        deferredColorAttachments.push_back(attachment);
    }
};

void DeferredRender::recordCommandList() {
    _commandList->beginCommandList();
    // start render pass
    _commandList->setRenderPass(_deferredRenderPass);
    // start G-Buffer pipeline
    _commandList->nextPipeline();
    // TODO replace with actual scene buffers 
    _commandList->setVertexBuffers({});
    // TODO replace with actual scene index buffer
    GAL::WdBuffer indexBuffer;
    _commandList->setIndexBuffer(indexBuffer);
    // Viewport is dynamic state
    _commandList->setViewport(_viewportProperties);
    // should bind descriptor sets here for the shader params
    // that were set earlier, can do it in the draw commands
    // or when setting the next pipeline,
    // next pipeline makes more sense if it can be done out of order
    _commandList->drawIndexed();

    // now do deferred pipeline
    _commandList->nextPipeline();
    _commandList->drawVertices(3); // full screen triangle

    _commandList->endRenderPass();

    _commandList->endCommandList();
};

void DeferredRender::drawFrame() {
    // wait for current frame to be finished before working 
    _graphicsLayer->waitForFences({frameInFlightFence});

    // acquire swapchain image here

    _graphicsLayer->resetFences({frameInFlightFence});

    // update uniform buffer with scene properties here

    // reset command list to add current frame commands 

    _commandList->resetCommandList();

    recordCommandList(); // record all commands for current pipelines 

    //_commandList->endCommandList();

    _commandList->execute(frameInFlightFence);

    // this is too simple of an abstraction 
    _graphicsLayer->presentCurrentFrame();
};

// draws one frame, maybe needs a better name 

void DeferredRender::render(Scene scene) {
    // create surfaces for rendering 
    _graphicsLayer->createRenderingSurfaces();
    // these should be in init for the first time, otherwise cache

    // Extract all shaders from scene 
    Shader::Shader gbufferVertexShader;
    Shader::Shader gbufferFragmentShader;
    Shader::Shader deferredVertexShader;
    Shader::Shader deferredFragmentShader;
    
    // Create vertex builder here from Vertex properties

    std::shared_ptr<GAL::WdVertexBuilder> gBufferBuilder = GAL::VertexBuilderManager::getManager()->getBuilder<GBufferVertexBuilder>();
    std::shared_ptr<GAL::WdVertexBuilder> deferredBuilder = GAL::VertexBuilderManager::getManager()->getBuilder<DeferredVertexBuilder>();


    _viewportProperties.startCoords = {0, 0};
    _viewportProperties.endCoords = swapchainExtent;
    _viewportProperties.scissor = {{0, 0}, swapchainExtent};
    
    // these should be friend classes as well 
    // need to say here to use depth... should prob be created in the underlying GAL 
    GAL::WdPipeline gBufferPipeline = _graphicsLayer->createPipeline(gbufferVertexShader, gbufferFragmentShader, gBufferBuilder.get(), _viewportProperties);

    GAL::WdPipeline deferredPipeline = _graphicsLayer->createPipeline(deferredVertexShader, deferredFragmentShader, deferredBuilder.get(), _viewportProperties);

    // ^^ this could be/should be cached somehow so we actually only look for these pipelines if they don't exist.
    std::vector<GAL::WdPipeline> pipelines = {gBufferPipeline, deferredPipeline};

    GAL::WdSize uboBufferSize;
    GAL::WdBuffer uboBuffer = _graphicsLayer->createBuffer(uboBufferSize, GAL::WdBufferUsage::WD_UNIFORM_BUFFER);

    gBufferPipeline.setVertexUniform("UniformBufferObject", GAL::WdPipeline::ShaderResource(&uboBuffer));

    // create depth & color resources for GBuffer Pass 
    createDeferredColorAttachments();
    createDepthAttachment();

    gBufferPipeline.setFragmentOutput("diffuseProperties", GAL::WdPipeline::ShaderResource(&deferredColorAttachments[0]));
    gBufferPipeline.setFragmentOutput("specularProperties", GAL::WdPipeline::ShaderResource(&deferredColorAttachments[1]));
    gBufferPipeline.setFragmentOutput("meshProperties", GAL::WdPipeline::ShaderResource(&deferredColorAttachments[2]));
    gBufferPipeline.setFragmentOutput("positionProperties", GAL::WdPipeline::ShaderResource(&deferredColorAttachments[3]));

    gBufferPipeline.setDepthStencilResource(GAL::WdPipeline::ShaderResource(depthAttachment));

    // need to set all texture arrays for GBuffer here as well 

    // set swapchain image here 
    deferredPipeline.setFragmentOutput("outColor", GAL::WdPipeline::ShaderResource());

    deferredPipeline.setFragmentUniform("diffuseInput", GAL::WdPipeline::ShaderResource(&deferredColorAttachments[0]));
    deferredPipeline.setFragmentUniform("specularInput", GAL::WdPipeline::ShaderResource(&deferredColorAttachments[1]));
    deferredPipeline.setFragmentUniform("meshInput", GAL::WdPipeline::ShaderResource(&deferredColorAttachments[2]));
    deferredPipeline.setFragmentUniform("positionInput", GAL::WdPipeline::ShaderResource(&deferredColorAttachments[3]));

    GAL::WdSize lightBufferSize;
    GAL::WdBuffer lightBuffer = _graphicsLayer->createBuffer(lightBufferSize, GAL::WdBufferUsage::WD_UNIFORM_BUFFER);
    
    deferredPipeline.setFragmentUniform("PointLight", GAL::WdPipeline::ShaderResource(&lightBuffer));

    GAL::WdSize cameraBufferSize;
    GAL::WdBuffer cameraBuffer = _graphicsLayer->createBuffer(cameraBufferSize, GAL::WdBufferUsage::WD_UNIFORM_BUFFER);
    
    deferredPipeline.setFragmentUniform("PointLight", GAL::WdPipeline::ShaderResource(&cameraBuffer));


    _deferredRenderPass = _graphicsLayer->createRenderPass(pipelines, attachments);

    // Go over scene and create/delete all textures models etc here 
    ///////////////////////////

    // Deal with texture sampler here too, should it be internal somehow? 

    // create descriptor sets, this is "SetShaderParam" wherever I decide to add that

    // create the command list (all the command buffers that will be used)
    _commandList = _graphicsLayer->createCommandList();

    frameInFlightFence = _graphicsLayer->createFence(); // fence signaled when reset 
};  

};