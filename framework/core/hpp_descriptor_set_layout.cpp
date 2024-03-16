/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

#include "hpp_descriptor_set_layout.h"

#include "hpp_device.h"
#include "hpp_physical_device.h"
#include "hpp_shader_module.h"

namespace vkb
{
namespace core
{

namespace
{
inline vk::DescriptorType find_descriptor_type(HPPShaderResourceType resource_type, bool dynamic)
{
	switch (resource_type)
	{
		case HPPShaderResourceType::InputAttachment:
			return vk::DescriptorType::eInputAttachment;
			break;
		case HPPShaderResourceType::Image:
			return vk::DescriptorType::eSampledImage;
			break;
		case HPPShaderResourceType::ImageSampler:
			return vk::DescriptorType::eCombinedImageSampler;
			break;
		case HPPShaderResourceType::ImageStorage:
			return vk::DescriptorType::eStorageImage;
			break;
		case HPPShaderResourceType::Sampler:
			return vk::DescriptorType::eSampler;
			break;
		case HPPShaderResourceType::BufferUniform:
			if (dynamic)
			{
				return vk::DescriptorType::eUniformBufferDynamic;
			}
			else
			{
				return vk::DescriptorType::eUniformBuffer;
			}
			break;
		case HPPShaderResourceType::BufferStorage:
			if (dynamic)
			{
				return vk::DescriptorType::eStorageBufferDynamic;
			}
			else
			{
				return vk::DescriptorType::eStorageBuffer;
			}
			break;
		default:
			throw std::runtime_error("No conversion possible for the shader resource type.");
			break;
	}
}

inline bool validate_binding(const vk::DescriptorSetLayoutBinding &binding, const std::vector<vk::DescriptorType> &blacklist)
{
	return !(std::find_if(blacklist.begin(), blacklist.end(), [binding](const vk::DescriptorType &type) { return type == binding.descriptorType; }) != blacklist.end());
}

inline bool validate_flags(const HPPPhysicalDevice &gpu, const std::vector<vk::DescriptorSetLayoutBinding> &bindings, const std::vector<vk::DescriptorBindingFlagsEXT> &flags)
{
	// Assume bindings are valid if there are no flags
	if (flags.empty())
	{
		return true;
	}

	// Binding count has to equal flag count as its a 1:1 mapping
	if (bindings.size() != flags.size())
	{
		LOGE("Binding count has to be equal to flag count.");
		return false;
	}

	return true;
}
}        // namespace

HPPDescriptorSetLayout::HPPDescriptorSetLayout(HPPDevice                            &device,
                                               const uint32_t                        set_index,
                                               const std::vector<HPPShaderModule *> &shader_modules,
                                               const std::vector<HPPShaderResource> &resource_set) :
    device{device},
    set_index{set_index},
    shader_modules{shader_modules}
{
	// NOTE: `shader_modules` is passed in mainly for hashing their handles in `request_resource`.
	//        This way, different pipelines (with different shaders / shader variants) will get
	//        different descriptor set layouts (incl. appropriate name -> binding lookups)

	for (auto &resource : resource_set)
	{
		// Skip shader resources whitout a binding point
		if (resource.type == HPPShaderResourceType::Input ||
		    resource.type == HPPShaderResourceType::Output ||
		    resource.type == HPPShaderResourceType::PushConstant ||
		    resource.type == HPPShaderResourceType::SpecializationConstant)
		{
			continue;
		}

		// Convert from ShaderResourceType to VkDescriptorType.
		auto descriptor_type = find_descriptor_type(resource.type, resource.mode == HPPShaderResourceMode::Dynamic);

		if (resource.mode == HPPShaderResourceMode::UpdateAfterBind)
		{
			binding_flags.push_back(vk::DescriptorBindingFlagBitsEXT::eUpdateAfterBind);
		}
		else
		{
			// When creating a descriptor set layout, if we give a structure to create_info.pNext, each binding needs to have a binding flag
			// (pBindings[i] uses the flags in pBindingFlags[i])
			// Adding 0 ensures the bindings that dont use any flags are mapped correctly.
			binding_flags.push_back({});
		}

		// Convert HPPShaderResource to VkDescriptorSetLayoutBinding
		vk::DescriptorSetLayoutBinding layout_binding{};

		layout_binding.binding         = resource.binding;
		layout_binding.descriptorCount = resource.array_size;
		layout_binding.descriptorType  = descriptor_type;
		layout_binding.stageFlags      = static_cast<vk::ShaderStageFlags>(resource.stages);

		bindings.push_back(layout_binding);

		// Store mapping between binding and the binding point
		bindings_lookup.emplace(resource.binding, layout_binding);

		binding_flags_lookup.emplace(resource.binding, binding_flags.back());

		resources_lookup.emplace(resource.name, resource.binding);
	}

	vk::DescriptorSetLayoutCreateInfo create_info{
	    {},
	    bindings,
	};
	create_info.bindingCount = to_u32(bindings.size());
	create_info.pBindings    = bindings.data();

	// Handle update-after-bind extensions
	vk::DescriptorSetLayoutBindingFlagsCreateInfoEXT binding_flags_create_info;
	if (std::find_if(resource_set.begin(), resource_set.end(),
	                 [](const HPPShaderResource &shader_resource) { return shader_resource.mode == HPPShaderResourceMode::UpdateAfterBind; }) != resource_set.end())
	{
		// Spec states you can't have ANY dynamic resources if you have one of the bindings set to update-after-bind
		if (std::find_if(resource_set.begin(), resource_set.end(),
		                 [](const HPPShaderResource &shader_resource) { return shader_resource.mode == HPPShaderResourceMode::Dynamic; }) != resource_set.end())
		{
			throw std::runtime_error("Cannot create descriptor set layout, dynamic resources are not allowed if at least one resource is update-after-bind.");
		}

		if (!validate_flags(device.get_gpu(), bindings, binding_flags))
		{
			throw std::runtime_error("Invalid binding, couldn't create descriptor set layout.");
		}

		binding_flags_create_info = vk::DescriptorSetLayoutBindingFlagsCreateInfoEXT{
		    binding_flags};
		binding_flags_create_info.bindingCount  = to_u32(binding_flags.size());
		binding_flags_create_info.pBindingFlags = binding_flags.data();

		create_info.pNext = &binding_flags_create_info;
		if (std::find(binding_flags.begin(), binding_flags.end(), vk::DescriptorBindingFlagBitsEXT::eUpdateAfterBind) != binding_flags.end())
		{
			create_info.flags |= vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPoolEXT;
		}
	}

	// Create the Vulkan descriptor set layout handle
	handle = device.get_handle().createDescriptorSetLayout(create_info);
}

HPPDescriptorSetLayout::HPPDescriptorSetLayout(HPPDescriptorSetLayout &&other) :
    device{other.device},
    shader_modules{other.shader_modules},
    handle{other.handle},
    set_index{other.set_index},
    bindings{std::move(other.bindings)},
    binding_flags{std::move(other.binding_flags)},
    bindings_lookup{std::move(other.bindings_lookup)},
    binding_flags_lookup{std::move(other.binding_flags_lookup)},
    resources_lookup{std::move(other.resources_lookup)}
{
	other.handle = VK_NULL_HANDLE;
}

HPPDescriptorSetLayout::~HPPDescriptorSetLayout()
{
	// Destroy descriptor set layout
	if (handle != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(device.get_handle(), handle, nullptr);
	}
}

vk::DescriptorSetLayout HPPDescriptorSetLayout::get_handle() const
{
	return handle;
}

const uint32_t HPPDescriptorSetLayout::get_index() const
{
	return set_index;
}

const std::vector<vk::DescriptorSetLayoutBinding> &HPPDescriptorSetLayout::get_bindings() const
{
	return bindings;
}

const std::vector<vk::DescriptorBindingFlagsEXT> &HPPDescriptorSetLayout::get_binding_flags() const
{
	return binding_flags;
}

std::unique_ptr<vk::DescriptorSetLayoutBinding> HPPDescriptorSetLayout::get_layout_binding(uint32_t binding_index) const
{
	auto it = bindings_lookup.find(binding_index);

	if (it == bindings_lookup.end())
	{
		return nullptr;
	}

	return std::make_unique<vk::DescriptorSetLayoutBinding>(it->second);
}

std::unique_ptr<vk::DescriptorSetLayoutBinding> HPPDescriptorSetLayout::get_layout_binding(const std::string &name) const
{
	auto it = resources_lookup.find(name);

	if (it == resources_lookup.end())
	{
		return nullptr;
	}

	return get_layout_binding(it->second);
}

vk::DescriptorBindingFlagsEXT HPPDescriptorSetLayout::get_layout_binding_flag(const uint32_t binding_index) const
{
	auto it = binding_flags_lookup.find(binding_index);

	if (it == binding_flags_lookup.end())
	{
		return {};
	}

	return it->second;
}

const std::vector<HPPShaderModule *> &HPPDescriptorSetLayout::get_shader_modules() const
{
	return shader_modules;
}

}        // namespace core
}        // namespace vkb
