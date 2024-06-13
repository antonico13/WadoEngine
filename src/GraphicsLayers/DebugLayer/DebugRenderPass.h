#ifndef WADO_DEBUG_RENDER_PASS
#define WADO_DEBUG_RENDER_PASS


#include "WdImage.h"
#include "WdPipeline.h"
#include "WdRenderPass.h"

#include <map>
#include <tuple>

namespace Wado::GAL::Debug { 

    class DebugRenderPass : public WdRenderPass {
        public:
            DebugRenderPass(const std::string& name);
        private:
    };
};

#endif