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
#include <core/hpp_device.h>
#include <core/hpp_image.h>
#include <core/hpp_descriptor_pool.h>
#include <core/hpp_descriptor_set.h>
#include <core/hpp_descriptor_set_layout.h>
#include <core/hpp_pipeline_layout.h>
#include <core/hpp_image_view.h>
#include <core/hpp_render_pass.h>
#include <core/hpp_shader_module.h>
#include <rendering/hpp_render_target.h>
#include <rendering/hpp_pipeline_state.h>


namespace std
{
template <typename Key, typename Value>
struct hash<std::map<Key, Value>>
{
	size_t operator()(std::map<Key, Value> const &bindings) const
	{
		size_t result = 0;
		vkb::hash_combine(result, bindings.size());
		for (auto const &binding : bindings)
		{
			vkb::hash_combine(result, binding.first);
			vkb::hash_combine(result, binding.second);
		}
		return result;
	}
};

template <typename T>
struct hash<std::vector<T>>
{
	size_t operator()(std::vector<T> const &values) const
	{
		size_t result = 0;
		vkb::hash_combine(result, values.size());
		for (auto const &value : values)
		{
			vkb::hash_combine(result, value);
		}
		return result;
	}
};

template <>
struct hash<vkb::common::HPPLoadStoreInfo>
{
	size_t operator()(vkb::common::HPPLoadStoreInfo const &lsi) const
	{
		size_t result = 0;
		vkb::hash_combine(result, lsi.load_op);
		vkb::hash_combine(result, lsi.store_op);
		return result;
	}
};

template <typename T>
struct hash<vkb::core::HPPVulkanResource<T>>
{
	size_t operator()(const vkb::core::HPPVulkanResource<T> &vulkan_resource) const
	{
		return std::hash<T>()(vulkan_resource.get_handle());
	}
};




template <>
struct hash<vkb::core::HPPShaderVariant>
{
	std::size_t operator()(const vkb::core::HPPShaderVariant &shader_variant) const
	{
		std::size_t result = 0;

		vkb::hash_combine(result, shader_variant.get_id());

		return result;
	}
};

template <>
struct hash<vkb::core::HPPDescriptorSetLayout>
{
	std::size_t operator()(const vkb::core::HPPDescriptorSetLayout &descriptor_set_layout) const
	{
		std::size_t result = 0;

		vkb::hash_combine(result, descriptor_set_layout.get_handle());

		return result;
	}
};

template <>
struct hash<vkb::core::HPPRenderPass>
{
	std::size_t operator()(const vkb::core::HPPRenderPass &render_pass) const
	{
		std::size_t result = 0;

		vkb::hash_combine(result, render_pass.get_handle());

		return result;
	}
};

template <>
struct hash<vkb::core::HPPDescriptorPool>
{
	std::size_t operator()(const vkb::core::HPPDescriptorPool &descriptor_pool) const
	{
		std::size_t result = 0;

		vkb::hash_combine(result, descriptor_pool.get_descriptor_set_layout());

		return result;
	}
};

template <>
struct hash<vkb::core::HPPShaderSource>
{
	std::size_t operator()(const vkb::core::HPPShaderSource &shader_source) const
	{
		std::size_t result = 0;

		vkb::hash_combine(result, shader_source.get_id());

		return result;
	}
};

template <>
struct hash<vkb::rendering::HPPStencilOpState>
{
	std::size_t operator()(const vkb::rendering::HPPStencilOpState &stencil) const
	{
		std::size_t result = 0;

		vkb::hash_combine(result, stencil.compare_op);
		vkb::hash_combine(result, stencil.depth_fail_op);
		vkb::hash_combine(result, stencil.fail_op);
		vkb::hash_combine(result, stencil.pass_op);

		return result;
	}
};

template <>
struct hash<vkb::rendering::HPPSpecializationConstantState>
{
	std::size_t operator()(const vkb::rendering::HPPSpecializationConstantState &specialization_constant_state) const
	{
		std::size_t result = 0;

		for (auto constants : specialization_constant_state.get_specialization_constant_state())
		{
			vkb::hash_combine(result, constants.first);
			for (const auto data : constants.second)
			{
				vkb::hash_combine(result, data);
			}
		}

		return result;
	}
};

template <>
struct hash<vkb::rendering::HPPColorBlendAttachmentState>
{
	std::size_t operator()(const vkb::rendering::HPPColorBlendAttachmentState &color_blend_attachment) const
	{
		std::size_t result = 0;

		vkb::hash_combine(result, color_blend_attachment.alpha_blend_op);
		vkb::hash_combine(result, color_blend_attachment.blend_enable);
		vkb::hash_combine(result, color_blend_attachment.color_blend_op);
		vkb::hash_combine(result, color_blend_attachment.color_write_mask);
		vkb::hash_combine(result, color_blend_attachment.dst_alpha_blend_factor);
		vkb::hash_combine(result, color_blend_attachment.dst_color_blend_factor);
		vkb::hash_combine(result, color_blend_attachment.src_alpha_blend_factor);
		vkb::hash_combine(result, color_blend_attachment.src_color_blend_factor);

		return result;
	}
};

template <>
struct hash<vkb::rendering::HPPPipelineState>
{
	std::size_t operator()(const vkb::rendering::HPPPipelineState &pipeline_state) const
	{
		std::size_t result = 0;

		vkb::hash_combine(result, pipeline_state.get_pipeline_layout().get_handle());

		// For graphics only
		if (auto render_pass = pipeline_state.get_render_pass())
		{
			vkb::hash_combine(result, render_pass->get_handle());
		}

		vkb::hash_combine(result, pipeline_state.get_specialization_constant_state());

		vkb::hash_combine(result, pipeline_state.get_subpass_index());

		for (auto shader_module : pipeline_state.get_pipeline_layout().get_shader_modules())
		{
			vkb::hash_combine(result, shader_module->get_id());
		}

		// VkPipelineVertexInputStateCreateInfo
		for (auto &attribute : pipeline_state.get_vertex_input_state().attributes)
		{
			vkb::hash_combine(result, attribute);
		}

		for (auto &binding : pipeline_state.get_vertex_input_state().bindings)
		{
			vkb::hash_combine(result, binding);
		}

		// VkPipelineInputAssemblyStateCreateInfo
		vkb::hash_combine(result, pipeline_state.get_input_assembly_state().primitive_restart_enable);
		vkb::hash_combine(result, static_cast<std::underlying_type<VkPrimitiveTopology>::type>(pipeline_state.get_input_assembly_state().topology));

		// VkPipelineViewportStateCreateInfo
		vkb::hash_combine(result, pipeline_state.get_viewport_state().viewport_count);
		vkb::hash_combine(result, pipeline_state.get_viewport_state().scissor_count);

		// VkPipelineRasterizationStateCreateInfo
		vkb::hash_combine(result, pipeline_state.get_rasterization_state().cull_mode);
		vkb::hash_combine(result, pipeline_state.get_rasterization_state().depth_bias_enable);
		vkb::hash_combine(result, pipeline_state.get_rasterization_state().depth_clamp_enable);
		vkb::hash_combine(result, pipeline_state.get_rasterization_state().front_face);
		vkb::hash_combine(result, pipeline_state.get_rasterization_state().polygon_mode);
		vkb::hash_combine(result, pipeline_state.get_rasterization_state().rasterizer_discard_enable);

		// VkPipelineMultisampleStateCreateInfo
		vkb::hash_combine(result, pipeline_state.get_multisample_state().alpha_to_coverage_enable);
		vkb::hash_combine(result, pipeline_state.get_multisample_state().alpha_to_one_enable);
		vkb::hash_combine(result, pipeline_state.get_multisample_state().min_sample_shading);
		vkb::hash_combine(result, static_cast<std::underlying_type<VkSampleCountFlagBits>::type>(pipeline_state.get_multisample_state().rasterization_samples));
		vkb::hash_combine(result, pipeline_state.get_multisample_state().sample_shading_enable);
		vkb::hash_combine(result, pipeline_state.get_multisample_state().sample_mask);

		// VkPipelineDepthStencilStateCreateInfo
		vkb::hash_combine(result, pipeline_state.get_depth_stencil_state().back);
		vkb::hash_combine(result, pipeline_state.get_depth_stencil_state().depth_bounds_test_enable);
		vkb::hash_combine(result, static_cast<std::underlying_type<VkCompareOp>::type>(pipeline_state.get_depth_stencil_state().depth_compare_op));
		vkb::hash_combine(result, pipeline_state.get_depth_stencil_state().depth_test_enable);
		vkb::hash_combine(result, pipeline_state.get_depth_stencil_state().depth_write_enable);
		vkb::hash_combine(result, pipeline_state.get_depth_stencil_state().front);
		vkb::hash_combine(result, pipeline_state.get_depth_stencil_state().stencil_test_enable);

		// VkPipelineColorBlendStateCreateInfo
		vkb::hash_combine(result, static_cast<std::underlying_type<VkLogicOp>::type>(pipeline_state.get_color_blend_state().logic_op));
		vkb::hash_combine(result, pipeline_state.get_color_blend_state().logic_op_enable);

		for (auto &attachment : pipeline_state.get_color_blend_state().attachments)
		{
			vkb::hash_combine(result, attachment);
		}

		return result;
	}
};

template <>
struct hash<vkb::core::HPPDescriptorSet>
{
	size_t operator()(vkb::core::HPPDescriptorSet &descriptor_set) const
	{
		size_t result = 0;
		vkb::hash_combine(result, descriptor_set.get_layout());
		// descriptor_pool ?
		vkb::hash_combine(result, descriptor_set.get_buffer_infos());
		vkb::hash_combine(result, descriptor_set.get_image_infos());
		vkb::hash_combine(result, descriptor_set.get_handle());
		// write_descriptor_sets ?

		return result;
	}
};

template <>
struct hash<vkb::core::HPPImage>
{
	size_t operator()(const vkb::core::HPPImage &image) const
	{
		size_t result = 0;
		vkb::hash_combine(result, image.get_memory());
		vkb::hash_combine(result, image.get_type());
		vkb::hash_combine(result, image.get_extent());
		vkb::hash_combine(result, image.get_format());
		vkb::hash_combine(result, image.get_usage());
		vkb::hash_combine(result, image.get_sample_count());
		vkb::hash_combine(result, image.get_tiling());
		vkb::hash_combine(result, image.get_subresource());
		vkb::hash_combine(result, image.get_array_layer_count());
		return result;
	}
};

template <>
struct hash<vkb::core::HPPImageView>
{
	size_t operator()(const vkb::core::HPPImageView &image_view) const
	{
		size_t result = std::hash<vkb::core::HPPVulkanResource<vk::ImageView>>()(image_view);
		vkb::hash_combine(result, image_view.get_image());
		vkb::hash_combine(result, image_view.get_format());
		vkb::hash_combine(result, image_view.get_subresource_range());
		return result;
	}
};

template <>
struct hash<vkb::core::HPPShaderResource>
{
	size_t operator()(vkb::core::HPPShaderResource const &shader_resource) const
	{
		size_t result = 0;
		vkb::hash_combine(result, shader_resource.stages);
		vkb::hash_combine(result, shader_resource.type);
		vkb::hash_combine(result, shader_resource.mode);
		vkb::hash_combine(result, shader_resource.set);
		vkb::hash_combine(result, shader_resource.binding);
		vkb::hash_combine(result, shader_resource.location);
		vkb::hash_combine(result, shader_resource.input_attachment_index);
		vkb::hash_combine(result, shader_resource.vec_size);
		vkb::hash_combine(result, shader_resource.columns);
		vkb::hash_combine(result, shader_resource.array_size);
		vkb::hash_combine(result, shader_resource.offset);
		vkb::hash_combine(result, shader_resource.size);
		vkb::hash_combine(result, shader_resource.constant_id);
		vkb::hash_combine(result, shader_resource.qualifiers);
		vkb::hash_combine(result, shader_resource.name);
		return result;
	}
};

template <>
struct hash<vkb::core::HPPSubpassInfo>
{
	size_t operator()(vkb::core::HPPSubpassInfo const &subpass_info) const
	{
		size_t result = 0;
		vkb::hash_combine(result, subpass_info.input_attachments);
		vkb::hash_combine(result, subpass_info.output_attachments);
		vkb::hash_combine(result, subpass_info.color_resolve_attachments);
		vkb::hash_combine(result, subpass_info.disable_depth_stencil_attachment);
		vkb::hash_combine(result, subpass_info.depth_stencil_resolve_attachment);
		vkb::hash_combine(result, subpass_info.depth_stencil_resolve_mode);
		vkb::hash_combine(result, subpass_info.debug_name);
		return result;
	}
};

template <>
struct hash<vkb::rendering::HPPAttachment>
{
	size_t operator()(const vkb::rendering::HPPAttachment &attachment) const
	{
		size_t result = 0;
		vkb::hash_combine(result, attachment.format);
		vkb::hash_combine(result, attachment.samples);
		vkb::hash_combine(result, attachment.usage);
		vkb::hash_combine(result, attachment.initial_layout);
		return result;
	}
};

template <>
struct hash<vkb::rendering::HPPRenderTarget>
{
	size_t operator()(const vkb::rendering::HPPRenderTarget &render_target) const
	{
		size_t result = 0;
		vkb::hash_combine(result, render_target.get_extent());
		for (auto const &view : render_target.get_views())
		{
			vkb::hash_combine(result, view);
		}
		for (auto const &attachment : render_target.get_attachments())
		{
			vkb::hash_combine(result, attachment);
		}
		for (auto const &input : render_target.get_input_attachments())
		{
			vkb::hash_combine(result, input);
		}
		for (auto const &output : render_target.get_output_attachments())
		{
			vkb::hash_combine(result, output);
		}
		return result;
	}
};

}        // namespace std

namespace vkb
{
namespace common
{
/**
 * @brief facade helper functions and structs around the functions and structs in common/resource_caching, providing a vulkan.hpp-based interface
 */

namespace
{
template <class T, class... A>
struct HPPRecordHelper
{
	size_t record(HPPResourceRecord & /*recorder*/, A &.../*args*/)
	{
		return 0;
	}

	void index(HPPResourceRecord & /*recorder*/, size_t /*index*/, T & /*resource*/)
	{}
};

template <class... A>
struct HPPRecordHelper<vkb::core::HPPShaderModule, A...>
{
	size_t record(HPPResourceRecord &recorder, A &...args)
	{
		return recorder.register_shader_module(args...);
	}

	void index(HPPResourceRecord &recorder, size_t index, vkb::core::HPPShaderModule &shader_module)
	{
		recorder.set_shader_module(index, shader_module);
	}
};

template <class... A>
struct HPPRecordHelper<vkb::core::HPPPipelineLayout, A...>
{
	size_t record(HPPResourceRecord &recorder, A &...args)
	{
		return recorder.register_pipeline_layout(args...);
	}

	void index(HPPResourceRecord &recorder, size_t index, vkb::core::HPPPipelineLayout &pipeline_layout)
	{
		recorder.set_pipeline_layout(index, pipeline_layout);
	}
};

template <class... A>
struct HPPRecordHelper<vkb::core::HPPRenderPass, A...>
{
	size_t record(HPPResourceRecord &recorder, A &...args)
	{
		return recorder.register_render_pass(args...);
	}

	void index(HPPResourceRecord &recorder, size_t index, vkb::core::HPPRenderPass &render_pass)
	{
		recorder.set_render_pass(index, render_pass);
	}
};

template <class... A>
struct HPPRecordHelper<vkb::core::HPPGraphicsPipeline, A...>
{
	size_t record(HPPResourceRecord &recorder, A &...args)
	{
		return recorder.register_graphics_pipeline(args...);
	}

	void index(HPPResourceRecord &recorder, size_t index, vkb::core::HPPGraphicsPipeline &graphics_pipeline)
	{
		recorder.set_graphics_pipeline(index, graphics_pipeline);
	}
};

template <typename T>
inline void hash_param(size_t &seed, const T &value)
{
	hash_combine(seed, value);
}


template <typename T, typename... Args>
inline void hash_param(size_t &seed, const T &first_arg, const Args &...args)
{
	hash_param(seed, first_arg);

	hash_param(seed, args...);
}

}        // namespace



template <class T, class... A>
T &request_resource(vkb::core::HPPDevice &device, vkb::HPPResourceRecord *recorder, std::unordered_map<size_t, std::unique_ptr<T>> &resources, A &...args)
{
	HPPRecordHelper<T, A...> record_helper;

	size_t hash{0U};
	hash_param(hash, args...);

	auto res_it = resources.find(hash);

	if (res_it != resources.end())
	{
		return *(res_it->second);
	}

	// If we do not have it already, create and cache it
	const char *res_type = typeid(T).name();
	size_t      res_id   = resources.size();

	LOGD("Building #{} cache object ({})", res_id, res_type);

// Only error handle in release
#ifndef DEBUG
	try
	{
#endif
		auto res_ins_it = resources.emplace(hash, std::make_unique<T>(device, args...));

		if (!res_ins_it.second)
		{
			throw std::runtime_error{std::string{"Insertion error for #"} + std::to_string(res_id) + "cache object (" + res_type + ")"};
		}

		res_it = res_ins_it.first;

		if (recorder)
		{
			size_t index = record_helper.record(*recorder, args...);
			record_helper.index(*recorder, index, *(res_it->second));
		}
#ifndef DEBUG
	}
	catch (const std::exception &e)
	{
		LOGE("Creation error for #{} cache object ({})", res_id, res_type);
		throw e;
	}
#endif

	return *(res_it->second);
}
}        // namespace common
}        // namespace vkb
