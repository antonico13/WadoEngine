#include "GraphicsAbstractionLayer.h"
#include "Shader.h"

#include "spirv.h"
#include "spirv_cross.hpp"

// get layout decorations for uniforms 
#define ADD_PARAM_DESC(RESTYPE, PARAMTYPE) \
for (const spirv_cross::Resource &resource : resources.RESTYPE) { \
            if ( ((ShaderParameterType::PARAMTYPE == ShaderParameterType::WD_SAMPLED_BUFFER) || (ShaderParameterType::PARAMTYPE == ShaderParameterType::WD_BUFFER_IMAGE)) && \
                    (spirvCompiler.get_type(resource.type_id).image.dim != Dim::DimBuffer) ) { \
                continue; \
            } \
            ShaderParameter param; \
            param.paramType = ShaderParameterType::PARAMTYPE; \
            param.decorationSet = spirvCompiler.get_decoration(resource.id, spv::DecorationDescriptorSet); \
            param.decorationBinding = spirvCompiler.get_decoration(resource.id, spv::DecorationBinding); \
            param.decorationLocation = spirvCompiler.get_decoration(resource.id, spv::DecorationLocation); \
            params.uniforms[resource.name.c_str()] = param; \
        };

namespace Wado::GAL { 

    WdPipeline::WdPipeline(Shader::ShaderByteCode vertexShader, Shader::ShaderByteCode fragmentShader, WdVertexBuilder* vertexBuilder, WdViewportProperties viewportProperties) {
        _vertexShader = vertexShader;
        _fragmentShader = fragmentShader;
        _fragmentParams = generateShaderParams(_fragmentShader);
        _vertexParams = generateShaderParams(_vertexShader);
    };

    ShaderParams WdPipeline::generateShaderParams(Shader::ShaderByteCode byteCode) {
        ShaderParams params;
        // create compiler object for the SPIR-V bytecode
        spirv_cross::Compiler spirvCompiler(move(byteCode));
        spirv_cross::ShaderResources resources = comp.get_shader_resources();

        // Check all non-subpass input or attachment params 

        ADD_PARAM_DESC(sampled_images, WD_SAMPLED_IMAGE);
        ADD_PARAM_DESC(separate_images, WD_TEXTURE_IMAGE);
        ADD_PARAM_DESC(storage_images, WD_BUFFER_IMAGE);
        ADD_PARAM_DESC(sampled_images, WD_SAMPLED_BUFFER);
        ADD_PARAM_DESC(separate_samplers, WD_SAMPLER);
        ADD_PARAM_DESC(uniform_buffers, WD_UNIFORM_BUFFER);
        ADD_PARAM_DESC(push_constant_buffers, WD_PUSH_CONSTANT);
        ADD_PARAM_DESC(storage_buffer, WD_STORAGE_BUFFER);

        // add all attachments/outputs (only fragment) 
        for (const spirv_cross::Resource &resource : resources.stage_outputs) {
            ShaderParameter param; 
            param.paramType = ShaderParameterType::WD_STAGE_OUTPUT;
            param.decorationSet = spirvCompiler.get_decoration(resource.id, spv::DecorationDescriptorSet); 
            param.decorationBinding = spirvCompiler.get_decoration(resource.id, spv::DecorationBinding); 
            param.decorationLocation = spirvCompiler.get_decoration(resource.id, spv::DecorationLocation); 
            params.outputs[resource.name.c_str()] = param; 
        };

        // add all inputs (only vertex)
        // TODO, not relevant right now 
        // but can extract the actual shape of the vertex inputs without using a vertex builder actually, much more useful 

        // add all subpass inputs (Vulkan only)
        for (const spirv_cross::Resource &resource : resources.subpass_inputs) {
            ShaderParameter param; 
            param.paramType = ShaderParameterType::WD_SUBPASS_INPUT;
            param.decorationSet = spirvCompiler.get_decoration(resource.id, spv::DecorationDescriptorSet); 
            param.decorationBinding = spirvCompiler.get_decoration(resource.id, spv::DecorationBinding); 
            param.decorationLocation = spirvCompiler.get_decoration(resource.id, spv::DecorationLocation); 
            param.decorationIndex = spirvCompiler.get_decoration(resource.id, spv::DecorationInputAttachmentIndex); 
            params.subpassInputs[resource.name.c_str()] = param; 
        };

        return params;
    };


    void WdPipeline::setVertexUniform(std::string paramName, ShaderResource resource) {
    
    };

    void WdPipeline::setFragmentUniform(std::string paramName, ShaderResource resource) {

    };

    void WdPipeline::setFragmentOutput(std::string paramName, ShaderResource resource) {

    };
        
}