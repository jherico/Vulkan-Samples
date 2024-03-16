/* Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <core/hpp_pipeline.h>

namespace vkb
{
enum class HPPResourceType
{
	ShaderModule,
	PipelineLayout,
	RenderPass,
	GraphicsPipeline
};

/**
 * @brief facade class around vkb::ResourceRecord, providing a vulkan.hpp-based interface
 *
 * See vkb::ResourceRecord for documentation
 */
class HPPResourceRecord 
{
	using HPPPipelineLayout = core::HPPPipelineLayout;
	using HPPRenderPass = core::HPPRenderPass;
	using HPPShaderModule = core::HPPShaderModule;
	using HPPShaderSource = core::HPPShaderSource;
	using HPPShaderVariant = core::HPPShaderVariant;
	using HPPSubpassInfo = core::HPPSubpassInfo;
	using HPPAttachment = rendering::HPPAttachment;
	using HPPLoadStoreInfo = common::HPPLoadStoreInfo;
	using HPPPipelineState = rendering::HPPPipelineState;
	using HPPGraphicsPipeline = core::HPPGraphicsPipeline;

  public:
	void set_data(const std::vector<uint8_t> &data);

	std::vector<uint8_t> get_data();

	const std::ostringstream &get_stream();

	size_t register_shader_module(vk::ShaderStageFlagBits stage,
	                              const HPPShaderSource   &glsl_source,
	                              const std::string    &entry_point,
	                              const HPPShaderVariant &shader_variant);

	size_t register_pipeline_layout(const std::vector<HPPShaderModule *> &shader_modules);

	size_t register_render_pass(const std::vector<HPPAttachment>    &attachments,
	                            const std::vector<HPPLoadStoreInfo> &load_store_infos,
	                            const std::vector<HPPSubpassInfo>      &subpasses);

	size_t register_graphics_pipeline(vk::PipelineCache pipeline_cache,
	                                  HPPPipelineState &pipeline_state);

	void set_shader_module(size_t index, const HPPShaderModule &shader_module);

	void set_pipeline_layout(size_t index, const HPPPipelineLayout &pipeline_layout);

	void set_render_pass(size_t index, const HPPRenderPass &render_pass);

	void set_graphics_pipeline(size_t index, const HPPGraphicsPipeline &graphics_pipeline);
  private:
	std::ostringstream stream;

	std::vector<size_t> shader_module_indices;

	std::vector<size_t> pipeline_layout_indices;

	std::vector<size_t> render_pass_indices;

	std::vector<size_t> graphics_pipeline_indices;

	std::unordered_map<const HPPShaderModule *, size_t> shader_module_to_index;
	std::unordered_map<const HPPPipelineLayout *, size_t> pipeline_layout_to_index;
	std::unordered_map<const HPPRenderPass *, size_t> render_pass_to_index;
	std::unordered_map<const HPPGraphicsPipeline *, size_t> graphics_pipeline_to_index;
};

#if 0


/**
 * @brief Writes Vulkan objects in a memory stream.
 */
class ResourceRecord
{
  public:

  private:
};
#endif

}        // namespace vkb
