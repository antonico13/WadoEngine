#ifndef WADO_DEBUG_PIPELINE_H
#define WADO_DEBUG_PIPELINE_H

#include "WdPipeline.h"
#include "Shader.h"

#include <string>
#include <vector>
#include <map>
#include <tuple>

namespace Wado::GAL::Debug {
    class DebugPipeline : public WdPipeline {
        public:

            void setUniform(const WdStage stage, const std::string& paramName, const WdShaderResource& resource) override;

            void setUniform(const WdStage stage, const std::string& paramName, const std::vector<WdShaderResource>& resources) override;

            void addToUniform(const WdStage stage, const std::string& paramName, const WdShaderResource& resource, int index = UNIFORM_END) override;

            void setFragmentOutput(const std::string& paramName, Memory::WdClonePtr<WdImage> image) override;

            void setDepthStencilResource(Memory::WdClonePtr<WdImage> image) override;
        
        private:
    };
};

#endif