/* Copyright (c) 2019-2022, Arm Limited and Contributors
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

#include "hpp_descriptor_pool.h"

#include "hpp_descriptor_set_layout.h"
#include "hpp_device.h"

namespace vkb
{
namespace core
{
HPPDescriptorPool::HPPDescriptorPool(HPPDevice                    &device,
                                     const HPPDescriptorSetLayout &descriptor_set_layout,
                                     uint32_t                      pool_size) :
    device{device},
    descriptor_set_layout{&descriptor_set_layout}
{
	const auto &bindings = descriptor_set_layout.get_bindings();

	std::map<vk::DescriptorType, std::uint32_t> descriptor_type_counts;

	// Count each type of descriptor set
	for (auto &binding : bindings)
	{
		descriptor_type_counts[binding.descriptorType] += binding.descriptorCount;
	}

	// Allocate pool sizes array
	pool_sizes.resize(descriptor_type_counts.size());

	auto pool_size_it = pool_sizes.begin();

	// Fill pool size for each descriptor type count multiplied by the pool size
	for (auto &it : descriptor_type_counts)
	{
		pool_size_it->type = it.first;

		pool_size_it->descriptorCount = it.second * pool_size;

		++pool_size_it;
	}

	pool_max_sets = pool_size;
}

HPPDescriptorPool::~HPPDescriptorPool()
{
	// Destroy all descriptor pools
	for (auto pool : pools)
	{
		vkDestroyDescriptorPool(device.get_handle(), pool, nullptr);
	}
}

void HPPDescriptorPool::reset()
{
	// Reset all descriptor pools
	for (auto pool : pools)
	{
		device.get_handle().resetDescriptorPool(pool, {});
	}

	// Clear internal tracking of descriptor set allocations
	std::fill(pool_sets_count.begin(), pool_sets_count.end(), 0);
	set_pool_mapping.clear();

	// Reset the pool index from which descriptor sets are allocated
	pool_index = 0;
}

const HPPDescriptorSetLayout &HPPDescriptorPool::get_descriptor_set_layout() const
{
	assert(descriptor_set_layout && "Descriptor set layout is invalid");
	return *descriptor_set_layout;
}

void HPPDescriptorPool::set_descriptor_set_layout(const HPPDescriptorSetLayout &set_layout)
{
	descriptor_set_layout = &set_layout;
}

vk::DescriptorSet HPPDescriptorPool::allocate()
{
	pool_index = find_available_pool(pool_index);

	// Increment allocated set count for the current pool
	++pool_sets_count[pool_index];

	VkDescriptorSetLayout set_layout = get_descriptor_set_layout().get_handle();

	VkDescriptorSetAllocateInfo alloc_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
	alloc_info.descriptorPool     = pools[pool_index];
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts        = &set_layout;

	vk::DescriptorSet handle;

	// Allocate a new descriptor set from the current pool
	try
	{
		handle = device.get_handle().allocateDescriptorSets(alloc_info)[0];
	}
	catch (const vk::Error &err)
	{
		// Decrement allocated set count for the current pool
		--pool_sets_count[pool_index];
		return {};
	}

	// Store mapping between the descriptor set and the pool
	set_pool_mapping.emplace(handle, pool_index);

	return handle;
}

vk::Result HPPDescriptorPool::free(vk::DescriptorSet descriptor_set)
{
	// Get the pool index of the descriptor set
	auto it = set_pool_mapping.find(descriptor_set);

	if (it == set_pool_mapping.end())
	{
		return vk::Result::eIncomplete;
	}

	auto desc_pool_index = it->second;

	// Free descriptor set from the pool
	device.get_handle().freeDescriptorSets(pools[desc_pool_index], descriptor_set);

	// Remove descriptor set mapping to the pool
	set_pool_mapping.erase(it);

	// Decrement allocated set count for the pool
	--pool_sets_count[desc_pool_index];

	// Change the current pool index to use the available pool
	pool_index = desc_pool_index;

	return vk::Result::eSuccess;
}

std::uint32_t HPPDescriptorPool::find_available_pool(std::uint32_t search_index)
{
	// Create a new pool
	if (pools.size() <= search_index)
	{
		// Check descriptor set layout and enable the required flags
		vk::DescriptorPoolCreateFlags create_flags;
		for (auto binding_flag : descriptor_set_layout->get_binding_flags())
		{
			if (binding_flag & vk::DescriptorBindingFlagBitsEXT::eUpdateAfterBind)
			{
				create_flags |= vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
				break;
			}
		}

		vk::DescriptorPool handle;
		try
		{
			// Create the Vulkan descriptor pool
			handle = device.get_handle().createDescriptorPool(vk::DescriptorPoolCreateInfo{
			    create_flags,
			    pool_max_sets,
			    pool_sizes,
			});
		}
		catch (const vk::Error &err)
		{
			return 0;
		}

		// Store internally the Vulkan handle
		pools.push_back(handle);

		// Add set count for the descriptor pool
		pool_sets_count.push_back(0);

		return search_index;
	}
	else if (pool_sets_count[search_index] < pool_max_sets)
	{
		return search_index;
	}

	// Increment pool index
	return find_available_pool(++search_index);
}
}        // namespace core
}        // namespace vkb
