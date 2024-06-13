#include "DebugLayer.h"
#include "DebugCommandList.h"

namespace Wado::GAL::Debug {
    // Returns ref to a WdImage that represents the screen, can only be used as a fragment output!
    Memory::WdClonePtr<WdImage> DebugLayer::getDisplayTarget() {
        return Memory::WdMainPtr<WdImage>(nullptr).getClonePtr();
    };

    Memory::WdClonePtr<WdImage> DebugLayer::create2DImage(WdExtent2D extent, uint32_t mipLevels, WdSampleCount sampleCount, WdFormat imageFormat, WdImageUsageFlags usageFlags, bool multiFrame) {
        return Memory::WdMainPtr<WdImage>(nullptr).getClonePtr();
    };
    
    WdFormat DebugLayer::findSupportedHardwareFormat(const std::vector<WdFormat>& formatCandidates, WdImageTiling tiling, WdFormatFeatureFlags features) {
        return Wado::GAL::WdFormat::WD_FORMAT_D24_UNORM_S8_UINT;
    };

    Memory::WdClonePtr<WdBuffer> DebugLayer::createBuffer(WdSize size, WdBufferUsageFlags usageFlags, bool multiFrame) {
        return Memory::WdMainPtr<WdBuffer>(nullptr).getClonePtr();
    };
    
    Memory::WdClonePtr<WdImage> DebugLayer::createTemp2DImage(bool multiFrame) {
        return Memory::WdMainPtr<WdImage>(nullptr).getClonePtr();
    };

    Memory::WdClonePtr<WdBuffer> DebugLayer::createTempBuffer(bool multiFrame) {
        return Memory::WdMainPtr<WdBuffer>(nullptr).getClonePtr();
    };

    WdSamplerHandle DebugLayer::createSampler(const WdTextureAddressMode& addressMode, WdFilterMode minFilter, WdFilterMode magFilter, WdFilterMode mipMapFilter) {
        return 0;
    };
    
    void DebugLayer::updateBuffer(const WdBuffer& buffer, void *data, WdSize offset, WdSize dataSize, int bufferIndex) {

    };

    void DebugLayer::openBuffer(WdBuffer& buffer, int bufferIndex) {

    };
    void DebugLayer::closeBuffer(WdBuffer& buffer, int bufferIndex) {

    };

    // Immediate functions
    void DebugLayer::copyBufferToImage(const WdBuffer& buffer, const WdImage& image, WdExtent2D extent, int resourceIndex) {

    };

    void DebugLayer::copyBuffer(const WdBuffer& srcBuffer, const WdBuffer& dstBuffer, WdSize size, int bufferIndex) {

    };

    WdFenceHandle DebugLayer::createFence(bool signaled) {
        return 0;
    };

    void DebugLayer::waitForFences(const std::vector<WdFenceHandle>& fences, bool waitAll, uint64_t timeout) {

    };

    void DebugLayer::resetFences(const std::vector<WdFenceHandle>& fences) {

    };
    
    Memory::WdClonePtr<WdPipeline> DebugLayer::createPipeline(const Shader::WdShader& vertexShader, const Shader::WdShader& fragmentShader, const WdViewportProperties& viewportProperties) {
        return Memory::WdMainPtr<WdPipeline>(nullptr).getClonePtr();
    };

    Memory::WdClonePtr<WdRenderPass> DebugLayer::createRenderPass(const std::vector<WdPipeline>& pipelines) {
        return Memory::WdMainPtr<WdRenderPass>(nullptr).getClonePtr();
    };

    Memory::WdClonePtr<WdCommandList> DebugLayer::createCommandList() {
        Debug::DebugCommandList *cmdList = static_cast<Debug::DebugCommandList *>(malloc(sizeof(DebugCommandList)));
        return _commandLists.emplace_back(cmdList).getClonePtr();// Memory::WdMainPtr<WdCommandList>(cmdList).getClonePtr();
    };

    void DebugLayer::executeCommandList(const WdCommandList& commandList, WdFenceHandle fenceToSignal) {

    };

    void DebugLayer::transitionImageUsage(const WdImage& image, WdImageUsage currentUsage, WdImageUsage nextUsage) {

    };

    void DebugLayer::displayCurrentTarget() {

    };
};