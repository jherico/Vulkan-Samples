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

#include <common/hpp_vk_common.h>

namespace vkb
{
namespace core
{
class HPPPipelineLayout;
class HPPRenderPass;
}

namespace rendering
{
struct HPPColorBlendAttachmentState
{
	vk::Bool32              blend_enable           = false;
	vk::BlendFactor         src_color_blend_factor = vk::BlendFactor::eOne;
	vk::BlendFactor         dst_color_blend_factor = vk::BlendFactor::eZero;
	vk::BlendOp             color_blend_op         = vk::BlendOp::eAdd;
	vk::BlendFactor         src_alpha_blend_factor = vk::BlendFactor::eOne;
	vk::BlendFactor         dst_alpha_blend_factor = vk::BlendFactor::eZero;
	vk::BlendOp             alpha_blend_op         = vk::BlendOp::eAdd;
	vk::ColorComponentFlags color_write_mask       = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
};

struct HPPColorBlendState
{
	vk::Bool32                                logic_op_enable = false;
	vk::LogicOp                               logic_op        = vk::LogicOp::eClear;
	std::vector<HPPColorBlendAttachmentState> attachments;
};

struct HPPInputAssemblyState
{
	vk::PrimitiveTopology topology                 = vk::PrimitiveTopology::eTriangleList;
	vk::Bool32            primitive_restart_enable = false;
};

struct HPPMultisampleState
{
	vk::SampleCountFlagBits rasterization_samples    = vk::SampleCountFlagBits::e1;
	vk::Bool32              sample_shading_enable    = false;
	float                   min_sample_shading       = 0.0f;
	vk::SampleMask          sample_mask              = 0;
	vk::Bool32              alpha_to_coverage_enable = false;
	vk::Bool32              alpha_to_one_enable      = false;
};

struct HPPRasterizationState
{
	vk::Bool32        depth_clamp_enable        = false;
	vk::Bool32        rasterizer_discard_enable = false;
	vk::PolygonMode   polygon_mode              = vk::PolygonMode::eFill;
	vk::CullModeFlags cull_mode                 = vk::CullModeFlagBits::eBack;
	vk::FrontFace     front_face                = vk::FrontFace::eCounterClockwise;
	vk::Bool32        depth_bias_enable         = false;
};

class HPPSpecializationConstantState 
{
  public:
	void reset();

	bool is_dirty() const;

	void clear_dirty();

	template <class T>
	void set_constant(uint32_t constant_id, const T &data);

	void set_constant(uint32_t constant_id, const std::vector<uint8_t> &data);

	void set_specialization_constant_state(const std::unordered_map<uint32_t, std::vector<uint8_t>> &state);

	const std::unordered_map<uint32_t, std::vector<uint8_t>> &get_specialization_constant_state() const;

  private:
	bool dirty{false};
	// Map tracking state of the Specialization Constants
	std::unordered_map<uint32_t, std::vector<uint8_t>> specialization_constant_state;
};

struct HPPStencilOpState
{
	vk::StencilOp fail_op       = vk::StencilOp::eReplace;
	vk::StencilOp pass_op       = vk::StencilOp::eReplace;
	vk::StencilOp depth_fail_op = vk::StencilOp::eReplace;
	vk::CompareOp compare_op    = vk::CompareOp::eNever;
};

struct HPPVertexInputState
{
	std::vector<vk::VertexInputBindingDescription>   bindings;
	std::vector<vk::VertexInputAttributeDescription> attributes;
};

struct HPPViewportState
{
	uint32_t viewport_count = 1;
	uint32_t scissor_count  = 1;
};


struct HPPDepthStencilState
{
	vk::Bool32        depth_test_enable        = true;
	vk::Bool32        depth_write_enable       = true;
	vk::CompareOp     depth_compare_op         = vk::CompareOp::eGreater;        // Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept
	vk::Bool32        depth_bounds_test_enable = false;
	vk::Bool32        stencil_test_enable      = false;
	HPPStencilOpState front;
	HPPStencilOpState back;
};

class HPPPipelineState 
{
  public:
	void reset();

	void set_pipeline_layout(core::HPPPipelineLayout &pipeline_layout);

	void set_render_pass(const core::HPPRenderPass &render_pass);

	void set_specialization_constant(uint32_t constant_id, const std::vector<uint8_t> &data);

	void set_vertex_input_state(const HPPVertexInputState &vertex_input_state);

	void set_input_assembly_state(const HPPInputAssemblyState &input_assembly_state);

	void set_rasterization_state(const HPPRasterizationState &rasterization_state);

	void set_viewport_state(const HPPViewportState &viewport_state);

	void set_multisample_state(const HPPMultisampleState &multisample_state);

	void set_depth_stencil_state(const HPPDepthStencilState &depth_stencil_state);

	void set_color_blend_state(const HPPColorBlendState &color_blend_state);

	void set_subpass_index(uint32_t subpass_index);

	const core::HPPPipelineLayout &get_pipeline_layout() const;

	const core::HPPRenderPass *get_render_pass() const;

	const HPPSpecializationConstantState &get_specialization_constant_state() const;

	const HPPVertexInputState &get_vertex_input_state() const;

	const HPPInputAssemblyState &get_input_assembly_state() const;

	const HPPRasterizationState &get_rasterization_state() const;

	const HPPViewportState &get_viewport_state() const;

	const HPPMultisampleState &get_multisample_state() const;

	const HPPDepthStencilState &get_depth_stencil_state() const;

	const HPPColorBlendState &get_color_blend_state() const;

	uint32_t get_subpass_index() const;

	bool is_dirty() const;

	void clear_dirty();

  private:
	bool dirty{false};

	core::HPPPipelineLayout *pipeline_layout{nullptr};

	const core::HPPRenderPass *render_pass{nullptr};

	HPPSpecializationConstantState specialization_constant_state{};

	HPPVertexInputState vertex_input_state{};

	HPPInputAssemblyState input_assembly_state{};

	HPPRasterizationState rasterization_state{};

	HPPViewportState viewport_state{};

	HPPMultisampleState multisample_state{};

	HPPDepthStencilState depth_stencil_state{};

	HPPColorBlendState color_blend_state{};

	uint32_t subpass_index{0U};
};

}        // namespace rendering
}        // namespace vkb
