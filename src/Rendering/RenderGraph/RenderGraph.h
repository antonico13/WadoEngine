#ifndef WADO_RENDERGRAPH_H
#define WADO_RENDERGRAPH_H

#include "RenderGraphShaderTypes.h"
#include <cstdint>
#include <functional>
#include <string>

namespace Wado::RenderGraph {

    using WdRDGExecuteCallbackFn = const std::function<void()>;

    class WdRenderGraphBuilder {
        public:
            WdRenderGraphBuilder();
            ~WdRenderGraphBuilder();

            // templated by param type 
            template <typename T>
            void addRenderPass(const std::string& passName, const T& params, WdRDGExecuteCallbackFn&& callback);

            
        private:
        
    };

};

#endif