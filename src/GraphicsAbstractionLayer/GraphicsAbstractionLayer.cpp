#include "GraphicsAbstractionLayer.h"
#include "Shader.h"

#include "spirv.h"
#include "spirv_cross.hpp"

// get layout decorations for uniforms 
// TODO: make this NOT a macro 
#define ADD_UNIFORM_DESC(RESTYPE, PARAMTYPE) \
for (const spirv_cross::Resource &resource : resources.RESTYPE) { \
            if ( ((ShaderParameterType::PARAMTYPE == ShaderParameterType::WD_SAMPLED_BUFFER) || (ShaderParameterType::PARAMTYPE == ShaderParameterType::WD_BUFFER_IMAGE)) && \
                    (spirvCompiler.get_type(resource.type_id).image.dim != Dim::DimBuffer) ) { \
                continue; \
            } \
            Uniform uniform; \
            uniform.paramType = ShaderParameterType::PARAMTYPE; \
            uniform.decorationSet = spirvCompiler.get_decoration(resource.id, spv::DecorationDescriptorSet); \
            uniform.decorationBinding = spirvCompiler.get_decoration(resource.id, spv::DecorationBinding); \
            uniform.decorationLocation = spirvCompiler.get_decoration(resource.id, spv::DecorationLocation); \
            uniform.resourceCount = spirvCompiler.get_type(resource.type_id).array[0]; \
            uniform.resources.resize(uniform.resourceCount); \
            uniforms[resource.name] = uniform; \
        };

namespace Wado::GAL { 

    // index by vec size first, then by base type 
    const WdFormat SPIRVtoWdFormat[][] = {
        {WdFormat::WD_FORMAT_UNDEFINED, WdFormat::WD_FORMAT_UNDEFINED, WdFormat::WD_FORMAT_R8_UNORM, WdFormat::WD_FORMAT_R8_SINT, WdFormat::WD_FORMAT_R8_UINT, WdFormat::WD_FORMAT_R16_SINT, WdFormat::WD_FORMAT_R16_UINT, WdFormat::WD_FORMAT_R32_SINT, WdFormat::WD_FORMAT_R32_UINT, WdFormat::WD_FORMAT_R64_SINT, WdFormat::WD_FORMAT_R64_UINT, WdFormat::WD_FORMAT_R32_UINT, WdFormat::WD_FORMAT_R16_SFLOAT, WdFormat::WD_FORMAT_R32_SFLOAT, WdFormat::WD_FORMAT_R64_SFLOAT,}, // size 1
        {WdFormat::WD_FORMAT_UNDEFINED, WdFormat::WD_FORMAT_UNDEFINED, WdFormat::WD_FORMAT_R8G8_UNORM, WdFormat::WD_FORMAT_R8G8_SINT, WdFormat::WD_FORMAT_R8G8_UINT, WdFormat::WD_FORMAT_R16G16_SINT, WdFormat::WD_FORMAT_R16G16_UINT, WdFormat::WD_FORMAT_R32G32_SINT, WdFormat::WD_FORMAT_R32G32_UINT, WdFormat::WD_FORMAT_R64G64_SINT, WdFormat::WD_FORMAT_R64G64_UINT, WdFormat::WD_FORMAT_R32G32_UINT, WdFormat::WD_FORMAT_R16G16_SFLOAT, WdFormat::WD_FORMAT_R32G32_SFLOAT, WdFormat::WD_FORMAT_R64G64_SFLOAT,}, // size 2
        {WdFormat::WD_FORMAT_UNDEFINED, WdFormat::WD_FORMAT_UNDEFINED, WdFormat::WD_FORMAT_R8G8B8_UNORM, WdFormat::WD_FORMAT_R8G8B8_SINT, WdFormat::WD_FORMAT_R8G8B8_UINT, WdFormat::WD_FORMAT_R16G16B16_SINT, WdFormat::WD_FORMAT_R16G16B16_UINT, WdFormat::WD_FORMAT_R32G32B32_SINT, WdFormat::WD_FORMAT_R32G32B32_UINT, WdFormat::WD_FORMAT_R64G64B64_SINT, WdFormat::WD_FORMAT_R64G64B64_UINT, WdFormat::WD_FORMAT_R32G32B32_UINT, WdFormat::WD_FORMAT_R16G16B16_SFLOAT, WdFormat::WD_FORMAT_R32G32B32_SFLOAT, WdFormat::WD_FORMAT_R64G64B64_SFLOAT,}, // size 3
        {WdFormat::WD_FORMAT_UNDEFINED, WdFormat::WD_FORMAT_UNDEFINED, WdFormat::WD_FORMAT_R8G8B8A8_UNORM, WdFormat::WD_FORMAT_R8G8B8A8_SINT, WdFormat::WD_FORMAT_R8G8B8A8_UINT, WdFormat::WD_FORMAT_R16G16B16A16_SINT, WdFormat::WD_FORMAT_R16G16B16A16_UINT, WdFormat::WD_FORMAT_R32G32B32A32_SINT, WdFormat::WD_FORMAT_R32G32B32A32_UINT, WdFormat::WD_FORMAT_R64G64B64A64_SINT, WdFormat::WD_FORMAT_R64G64B64A64_UINT, WdFormat::WD_FORMAT_R32G32B32A32_UINT, WdFormat::WD_FORMAT_R16G16B16A16_SFLOAT, WdFormat::WD_FORMAT_R32G32B32A32_SFLOAT, WdFormat::WD_FORMAT_R64G64B64A64_SFLOAT,}, // size 4
    };

    // size in bytes 
    const size_t SPIRVBaseToSize[] = {
        0, // Unknown
		0, // Void
		4, //Boolean
		4, //SByte,
		4, //UByte,
		4, //Short,
		4, //UShort,
		4, //Int,
		4, //UInt,
		8, //Int64,
		8, //UInt64,
		4, //AtomicCounter,
		4, //Half,
		4, //Float,
		8, //Double,
    };

    WdPipeline::WdPipeline(Shader::ShaderByteCode vertexShader, Shader::ShaderByteCode fragmentShader, WdVertexBuilder* vertexBuilder, WdViewportProperties viewportProperties) {
        _vertexShader = vertexShader;
        _fragmentShader = fragmentShader;
        generateVertexParams(_vertexShader, vertexInputs, vertexUniforms);
        generateFragmentParams(_fragmentShader, subpassInputs, fragmentUniforms, fragmentOutputs);
        _viewportProperties = viewportProperties;
    };

    void WdPipeline::generateVertexParams(Shader::ShaderByteCode byteCode, VertexInputs& inputs, Uniforms& uniforms) {
        // create compiler object for the SPIR-V bytecode
        spirv_cross::Compiler spirvCompiler(move(byteCode));
        spirv_cross::ShaderResources resources = comp.get_shader_resources();

        // Check all non-subpass input uniforms 
        ADD_UNIFORM_DESC(sampled_images, WD_SAMPLED_IMAGE);
        ADD_UNIFORM_DESC(separate_images, WD_TEXTURE_IMAGE);
        ADD_UNIFORM_DESC(storage_images, WD_BUFFER_IMAGE);
        ADD_UNIFORM_DESC(sampled_images, WD_SAMPLED_BUFFER);
        ADD_UNIFORM_DESC(separate_samplers, WD_SAMPLER);
        ADD_UNIFORM_DESC(uniform_buffers, WD_UNIFORM_BUFFER);
        ADD_UNIFORM_DESC(push_constant_buffers, WD_PUSH_CONSTANT);
        ADD_UNIFORM_DESC(storage_buffer, WD_STORAGE_BUFFER);

        // process vertex input now 
        for (const spirv_cross::Resource &resource : resources.stage_inputs) {
            VertexInput vertexInput;
            vertexInput.paramType = ShaderParameterType::WD_STAGE_INPUT;
            vertexInput.decorationBinding = spirvCompiler.get_decoration(resource.id, spv::DecorationBinding); 
            vertexInput.decorationLocation = spirvCompiler.get_decoration(resource.id, spv::DecorationLocation); 
            vertexInput.offset = spirvCompiler.get_decoration(resource.id, spv::DecorationOffset); 

            const spirv_cross::SPIRType& type = spirvCompiler.get_type(resource.type_id);
            uint32_t vecSize = type.vecsize;
            spirv_cross::BaseType baseType = type.basetype;

            vertexInput.size = vecSize * SPIRVBaseToSize[baseType];
            vertexInput.format = SPIRVtoWdFormat[vecSize - 1][baseType];

            inputs.push_back(vertexInput);
        };
    };
    
    void WdPipeline::generateFragmentParams(Shader::ShaderByteCode byteCode, SubpassInputs& inputs, Uniforms& uniforms, FragmentOutputs& outputs) {
        // create compiler object for the SPIR-V bytecode
        spirv_cross::Compiler spirvCompiler(move(byteCode));
        spirv_cross::ShaderResources resources = comp.get_shader_resources();

        // Check all non-subpass input uniforms 
        ADD_UNIFORM_DESC(sampled_images, WD_SAMPLED_IMAGE);
        ADD_UNIFORM_DESC(separate_images, WD_TEXTURE_IMAGE);
        ADD_UNIFORM_DESC(storage_images, WD_BUFFER_IMAGE);
        ADD_UNIFORM_DESC(sampled_images, WD_SAMPLED_BUFFER);
        ADD_UNIFORM_DESC(separate_samplers, WD_SAMPLER);
        ADD_UNIFORM_DESC(uniform_buffers, WD_UNIFORM_BUFFER);
        ADD_UNIFORM_DESC(push_constant_buffers, WD_PUSH_CONSTANT);
        ADD_UNIFORM_DESC(storage_buffer, WD_STORAGE_BUFFER);

        // process supbass inputs now 
        for (const spirv_cross::Resource &resource : resources.subpass_inputs) {
            SubpassInput subpassInput;
            subpassInput.paramType = ShaderParameterType::WD_SUBPASS_INPUT;
            subpassInput.decorationBinding = spirvCompiler.get_decoration(resource.id, spv::DecorationBinding); 
            subpassInput.decorationLocation = spirvCompiler.get_decoration(resource.id, spv::DecorationLocation);
            subpassInput.decorationIndex = spirvCompiler.get_decoration(resource.id, spv::DecorationInputAttachmentIndex);
            subpassInput.decorationSet = spirvCompiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            // by construction the names have to be unique 
            inputs[resource.name] = subpassInput; 
        };

        // process fragment outputs now 
        for (const spirv_cross::Resource &resource : resources.subpass_inputs) {
            FragmentOutput fragOutput;
            fragOutput.paramType = ShaderParameterType::WD_STAGE_OUTPUT;
            fragOutput.decorationLocation = spirvCompiler.get_decoration(resource.id, spv::DecorationLocation);
            // by construction the names have to be unique 
            outputs[resource.name] = fragOutput; 
        };
    };

    // TODO: error handling here 
    void WdPipeline::setVertexUniform(const std::string& paramName, ShaderResource resource) {
        setUniform(vertexUniforms, paramName, {resource});
    };
    
    void WdPipeline::setFragmentUniform(const std::string& paramName, ShaderResource resource) {
        setUniform(fragmentUniforms, paramName, {resource});
    };

    void WdPipeline::setVertexUniform(const std::string& paramName, std::vector<ShaderResource>& resources) {
        setUniform(vertexUniforms, paramName, resources);
    };
    
    void WdPipeline::setFragmentUniform(const std::string& paramName, std::vector<ShaderResource>& resources) {
        setUniform(fragmentUniforms, paramName, resources);
    };

    void WdPipeline::setUniform(Uniforms& uniforms, const std::string& paramName, std::vector<ShaderResource>& resources) {
        // TODO: need to handle uniform aliasing here
        Uniforms::iterator it = uniforms.find(paramName);
        if (it != uniforms.end()) { // found, can set resource 
            if (it->second.resourceCount < resources.size()) {
                // throw error here...
                return;
            }
            it->second.resources = resources; // need to handle uniform aliasing here too
        }
        // need to throw error here
    };

    void WdPipeline::addToVertexUniform(const std::string& paramName, ShaderResource resource, int index) {
        addToUniform(vertexUniforms, paramName, resource, index);
    };
    
    void WdPipeline::addToFragmentUniform(const std::string& paramName, ShaderResource resource, int index) {
        addToUniform(fragmentUniforms, paramName, resource, index);
    };

    void WdPipeline::addToUniform(Uniforms& uniforms, const std::string& paramName, ShaderResource resource, int index) {
        // TODO: need to handle uniform aliasing here
        Uniforms::iterator it = uniforms.find(paramName);
        if (it != uniforms.end()) { // found, can set resource 
            if (it->second.resourceCount < index) {
                // throw error here...
                return;
            }
            if (index == UNIFORM_END) {
                if (it->second.resourceCount <= it->second.resources.size) {
                    // throw error here...
                    return; 
                }
                it->second.resources.push_back(resource);
            } else {
                if (it->second.resources.size() <= index) {
                    it->second.resources.resize(index + 1);
                }
                it->second.resources[index] = resource; // need to handle uniform aliasing here too, as well as typing
            }
        // need to throw error here 
        }
    };

    void WdPipeline::setFragmentOutput(const std::string& paramName, WdImageResource resource) {
        FragmentOutputs::iterator it = fragmentOutputs.find(paramName);
        if (it != fragmentOutputs.end()) {
            it->second.resource = resource;
        }
        // error, param not found bla bla 
        // can also type check here (later)
    };
    
    void WdPipeline::setDepthStencilResource(ShaderResource resource) {
        // should do some checks here for formats and what not, aliasing, etc.
        depthStencilResource = resource;
    };

}