#include "GraphicsAbstractionLayer.h"
#include "Shader.h"

#include "spirv.h"
#include "spirv_cross.hpp"

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

    WdPipeline::WdPipeline(Shader::ShaderByteCode vertexShader, Shader::ShaderByteCode fragmentShader, WdViewportProperties viewportProperties) : 
        _vertexShader(vertexShader),
        _fragmentShader(fragmentShader), 
        _viewportProperties(viewportProperties) {

        generateVertexParams();
        generateFragmentParams();
    };

    void WdPipeline::addUniformDescription(spirv_cross::Compiler& spirvCompiler, const std::vector<spirv_cross::Resource>& resources, const ShaderParameterType paramType, const WdStage stage) {
        for (const spirv_cross::Resource &resource : resources) {
            if ( ((paramType == ShaderParameterType::WD_SAMPLED_BUFFER) || (paramType == ShaderParameterType::WD_BUFFER_IMAGE)) &&
                    (spirvCompiler.get_type(resource.type_id).image.dim != Dim::DimBuffer) ) {
                continue;
            };

            Uniform uniform; 
            uniform.paramType = paramType; 
            uniform.decorationSet = spirvCompiler.get_decoration(resource.id, spv::DecorationDescriptorSet); 
            uniform.decorationBinding = spirvCompiler.get_decoration(resource.id, spv::DecorationBinding); 
            uniform.decorationLocation = spirvCompiler.get_decoration(resource.id, spv::DecorationLocation); 
            uniform.resourceCount = spirvCompiler.get_type(resource.type_id).array[0]; 
            uniform.resources.resize(uniform.resourceCount); 

            UniformAddress address(uniform.decorationSet, uniform.decorationBinding, uniform.decorationLocation); 
            _uniformAddresses[resource.name + std::to_string(stage)] = address; 
            // This could be an unnecessary write, but it's probably better than doing a std::find since that is log n 
            _uniforms[address] = uniform;
        };
    };

    void WdPipeline::generateVertexParams() {
        // create compiler object for the SPIR-V bytecode
        spirv_cross::Compiler spirvCompiler(move(_vertexShader));
        spirv_cross::ShaderResources resources = comp.get_shader_resources();

        // Check all non-subpass input uniforms 
        addUniformDescription(spirvCompiler, resources.sampled_images, ShaderParameterType::WD_SAMPLED_IMAGE, WdStage::Vertex);
        addUniformDescription(spirvCompiler, resources.separate_images, ShaderParameterType::WD_TEXTURE_IMAGE, WdStage::Vertex);
        addUniformDescription(spirvCompiler, resources.storage_images, ShaderParameterType::WD_BUFFER_IMAGE, WdStage::Vertex);
        addUniformDescription(spirvCompiler, resources.sampled_images, ShaderParameterType::WD_SAMPLED_BUFFER, WdStage::Vertex);
        addUniformDescription(spirvCompiler, resources.separate_samplers, ShaderParameterType::WD_SAMPLER, WdStage::Vertex);
        addUniformDescription(spirvCompiler, resources.uniform_buffers, ShaderParameterType::WD_UNIFORM_BUFFER, WdStage::Vertex);
        //addUniformDescription(push_constant_buffers, WD_PUSH_CONSTANT); TODO: These need to be handled separately 
        addUniformDescription(spirvCompiler, resources.storage_buffer, ShaderParameterType::WD_STORAGE_BUFFER, WdStage::Vertex);

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

            _vertexInputs.push_back(vertexInput);
        };
    };
    
    void WdPipeline::generateFragmentParams() {
        // create compiler object for the SPIR-V bytecode
        spirv_cross::Compiler spirvCompiler(move(_fragmentShader));
        spirv_cross::ShaderResources resources = comp.get_shader_resources();

        // Check all non-subpass input uniforms 
        addUniformDescription(spirvCompiler, resources.sampled_images, ShaderParameterType::WD_SAMPLED_IMAGE, WdStage::Fragment);
        addUniformDescription(spirvCompiler, resources.separate_images, ShaderParameterType::WD_TEXTURE_IMAGE, WdStage::Fragment);
        addUniformDescription(spirvCompiler, resources.storage_images, ShaderParameterType::WD_BUFFER_IMAGE, WdStage::Fragment);
        addUniformDescription(spirvCompiler, resources.sampled_images, ShaderParameterType::WD_SAMPLED_BUFFER, WdStage::Fragment);
        addUniformDescription(spirvCompiler, resources.separate_samplers, ShaderParameterType::WD_SAMPLER, WdStage::Fragment);
        addUniformDescription(spirvCompiler, resources.uniform_buffers, ShaderParameterType::WD_UNIFORM_BUFFER, WdStage::Fragment);
        //addUniformDescription(push_constant_buffers, WD_PUSH_CONSTANT); TODO: These need to be handled separately 
        addUniformDescription(spirvCompiler, resources.storage_buffer, ShaderParameterType::WD_STORAGE_BUFFER, WdStage::Fragment);

        // process supbass inputs now 
        for (const spirv_cross::Resource &resource : resources.subpass_inputs) {
            SubpassInput subpassInput;
            subpassInput.paramType = ShaderParameterType::WD_SUBPASS_INPUT;
            subpassInput.decorationBinding = spirvCompiler.get_decoration(resource.id, spv::DecorationBinding); 
            subpassInput.decorationLocation = spirvCompiler.get_decoration(resource.id, spv::DecorationLocation);
            subpassInput.decorationIndex = spirvCompiler.get_decoration(resource.id, spv::DecorationInputAttachmentIndex);
            subpassInput.decorationSet = spirvCompiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            // by construction the names have to be unique 
            _subpassInputs[resource.name] = subpassInput; 
        };

        // process fragment outputs now 
        for (const spirv_cross::Resource &resource : resources.subpass_inputs) {
            FragmentOutput fragOutput;
            fragOutput.paramType = ShaderParameterType::WD_STAGE_OUTPUT;
            fragOutput.decorationIndex = spirvCompiler.get_decoration(resource.id, spv::DecorationLocation);
            // by construction the names have to be unique 
            _fragmentOutputs[resource.name] = fragOutput; 
        };
    };

    // subpass inputs handled in setUniform 
    void WdPipeline::setUniform(const WdStage stage, const std::string& paramName, ShaderResource resource);

    // for array params
    void WdPipeline::setUniform(const WdStage stage, const std::string& paramName, std::vector<ShaderResource>& resources);

    void WdPipeline::addToUniform(const WdStage stage, const std::string& paramName, ShaderResource resource, int index = UNIFORM_END);


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