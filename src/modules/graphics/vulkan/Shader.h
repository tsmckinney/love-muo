/**
 * Copyright (c) 2006-2024 LOVE Development Team
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 **/

#pragma once

// LÖVE
#include "common/Optional.h"
#include "graphics/Shader.h"
#include "graphics/vulkan/ShaderStage.h"
#include "Vulkan.h"

// Libraries
#include "VulkanWrapper.h"
#include "libraries/spirv_cross/spirv_reflect.hpp"
#include "libraries/xxHash/xxhash.h"

// C++
#include <map>
#include <memory>
#include <unordered_map>
#include <queue>
#include <set>


namespace love
{
namespace graphics
{
namespace vulkan
{

struct GraphicsPipelineConfiguration
{
	VkRenderPass renderPass;
	VertexAttributes vertexAttributes;
	bool wireFrame;
	BlendState blendState;
	ColorChannelMask colorChannelMask;
	VkSampleCountFlagBits msaaSamples;
	uint32_t numColorAttachments;
	PrimitiveType primitiveType;
	uint64 packedColorAttachmentFormats;

	struct DynamicState
	{
		CullMode cullmode = CULL_NONE;
		Winding winding = WINDING_MAX_ENUM;
		StencilAction stencilAction = STENCIL_MAX_ENUM;
		CompareMode stencilCompare = COMPARE_MAX_ENUM;
		DepthState depthState{};
	} dynamicState;

	GraphicsPipelineConfiguration()
	{
		memset(this, 0, sizeof(GraphicsPipelineConfiguration));
	}

	bool operator==(const GraphicsPipelineConfiguration &other) const
	{
		return memcmp(this, &other, sizeof(GraphicsPipelineConfiguration)) == 0;
	}
};

struct GraphicsPipelineConfigurationHasher
{
	size_t operator() (const GraphicsPipelineConfiguration &configuration) const
	{
		return XXH32(&configuration, sizeof(GraphicsPipelineConfiguration), 0);
	}
};

class Graphics;

class Shader final
	: public graphics::Shader
	, public Volatile
{
public:

	struct AttributeInfo
	{
		int index;
		DataBaseType baseType;
	};

	Shader(StrongRef<love::graphics::ShaderStage> stages[], const CompileOptions &options);
	virtual ~Shader();

	bool loadVolatile() override;
	void unloadVolatile() override;

	VkPipeline getComputePipeline() const;

	const std::vector<VkPipelineShaderStageCreateInfo> &getShaderStages() const;

	const VkPipelineLayout getGraphicsPipelineLayout() const;

	void newFrame();

	void cmdPushDescriptorSets(VkCommandBuffer, VkPipelineBindPoint);

	void attach() override;

	ptrdiff_t getHandle() const override { return 0; }

	std::string getWarnings() const override { return ""; }

	int getVertexAttributeIndex(const std::string &name) override;
	const std::unordered_map<std::string, AttributeInfo> getVertexAttributeIndices() const { return attributes; }

	const UniformInfo *getUniformInfo(BuiltinUniform builtin) const override;

	void updateUniform(const UniformInfo *info, int count) override;

	void sendTextures(const UniformInfo *info, graphics::Texture **textures, int count) override;
	void sendBuffers(const UniformInfo *info, love::graphics::Buffer **buffers, int count) override;

	void setVideoTextures(graphics::Texture *ytexture, graphics::Texture *cbtexture, graphics::Texture *crtexture) override;

	void setMainTex(graphics::Texture *texture);

	VkPipeline getCachedGraphicsPipeline(Graphics *vgfx, const GraphicsPipelineConfiguration &configuration);

private:
	void compileShaders();
	void createDescriptorSetLayout();
	void createPipelineLayout();
	void createDescriptorPoolSizes();
	void buildLocalUniforms(spirv_cross::Compiler &comp, const spirv_cross::SPIRType &type, size_t baseoff, const std::string &basename);
	void createDescriptorPool();
	VkDescriptorSet allocateDescriptorSet();

	void setTextureDescriptor(const UniformInfo *info, love::graphics::Texture *texture, int index);
	void setBufferDescriptor(const UniformInfo *info, love::graphics::Buffer *buffer, int index);

	VkPipeline computePipeline = VK_NULL_HANDLE;

	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorPoolSize> descriptorPoolSizes;

	std::vector<std::vector<VkDescriptorPool>> descriptorPools;

	std::vector<VkDescriptorBufferInfo> descriptorBuffers;
	std::vector<VkDescriptorImageInfo> descriptorImages;
	std::vector<VkBufferView> descriptorBufferViews;
	std::vector<VkWriteDescriptorSet> descriptorWrites;

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	std::vector<VkShaderModule> shaderModules;

	Graphics *vgfx = nullptr;
	VkDevice device = VK_NULL_HANDLE;

	bool isCompute = false;
	bool resourceDescriptorsDirty = false;
	VkDescriptorSet currentDescriptorSet = VK_NULL_HANDLE;

	UniformInfo *builtinUniformInfo[BUILTIN_MAX_ENUM];

	std::unique_ptr<StreamBuffer> uniformBufferObjectBuffer;
	std::vector<uint8> localUniformData;
	std::vector<uint8> localUniformStagingData;
	uint32_t localUniformLocation = 0;
	OptionalInt builtinUniformDataOffset;

	std::unordered_map<std::string, AttributeInfo> attributes;

	std::unordered_map<GraphicsPipelineConfiguration, VkPipeline, GraphicsPipelineConfigurationHasher> graphicsPipelines;

	uint32_t currentFrame = 0;
	uint32_t currentDescriptorPool = 0;
};

}
}
}
