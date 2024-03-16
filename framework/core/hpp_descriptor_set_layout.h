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

#include "common/hpp_vk_common.h"

namespace vkb
{
namespace core
{

/**
 * @brief facade class around vkb::HPPDescriptorSetLayout, providing a vulkan.hpp-based interface
 *
 * See vkb::HPPDescriptorSetLayout for documentation
 */
class HPPDescriptorSetLayout
{
  public:
	/**
	 * @brief Creates a descriptor set layout from a set of resources
	 * @param device A valid Vulkan device
	 * @param set_index The descriptor set index this layout maps to
	 * @param shader_modules The shader modules this set layout will be used for
	 * @param resource_set A grouping of shader resources belonging to the same set
	 */
	HPPDescriptorSetLayout(
	    HPPDevice                            &device,
	    const uint32_t                        set_index,
	    const std::vector<HPPShaderModule *> &shader_modules,
	    const std::vector<HPPShaderResource> &resource_set);

	HPPDescriptorSetLayout(const HPPDescriptorSetLayout &) = delete;

	HPPDescriptorSetLayout(HPPDescriptorSetLayout &&other);

	~HPPDescriptorSetLayout();

	HPPDescriptorSetLayout &operator=(const HPPDescriptorSetLayout &) = delete;

	HPPDescriptorSetLayout &operator=(HPPDescriptorSetLayout &&) = delete;

	vk::DescriptorSetLayout get_handle() const;

	const uint32_t get_index() const;

	const std::vector<vk::DescriptorSetLayoutBinding> &get_bindings() const;

	std::unique_ptr<vk::DescriptorSetLayoutBinding> get_layout_binding(const uint32_t binding_index) const;

	std::unique_ptr<vk::DescriptorSetLayoutBinding> get_layout_binding(const std::string &name) const;

	const std::vector<vk::DescriptorBindingFlagsEXT> &get_binding_flags() const;

	vk::DescriptorBindingFlagsEXT get_layout_binding_flag(const uint32_t binding_index) const;

	const std::vector<HPPShaderModule *> &get_shader_modules() const;

  private:
	HPPDevice &device;

	vk::DescriptorSetLayout handle{VK_NULL_HANDLE};

	const uint32_t set_index;

	std::vector<vk::DescriptorSetLayoutBinding> bindings;

	std::vector<vk::DescriptorBindingFlagsEXT> binding_flags;

	std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> bindings_lookup;

	std::unordered_map<uint32_t, vk::DescriptorBindingFlagsEXT> binding_flags_lookup;

	std::unordered_map<std::string, uint32_t> resources_lookup;

	std::vector<HPPShaderModule *> shader_modules;
};
}        // namespace core
}        // namespace vkb
