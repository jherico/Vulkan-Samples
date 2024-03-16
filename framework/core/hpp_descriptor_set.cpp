/* Copyright (c) 2019-2024, Arm Limited and Contributors
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

#include "hpp_descriptor_set.h"

#include "common/hpp_resource_caching.h"
#include "core/hpp_descriptor_pool.h"
#include "core/hpp_descriptor_set_layout.h"
#include "core/hpp_device.h"

namespace vkb
{
namespace core
{
HPPDescriptorSet::HPPDescriptorSet(HPPDevice                                  &device,
                                   const HPPDescriptorSetLayout               &descriptor_set_layout,
                                   HPPDescriptorPool                          &descriptor_pool,
                                   const BindingMap<vk::DescriptorBufferInfo> &buffer_infos,
                                   const BindingMap<vk::DescriptorImageInfo>  &image_infos) :
    device{device},
    descriptor_set_layout{descriptor_set_layout},
    descriptor_pool{descriptor_pool},
    buffer_infos{buffer_infos},
    image_infos{image_infos},
    handle{descriptor_pool.allocate()}
{
	prepare();
}

void HPPDescriptorSet::reset(const BindingMap<vk::DescriptorBufferInfo> &new_buffer_infos, const BindingMap<vk::DescriptorImageInfo> &new_image_infos)
{
	if (!new_buffer_infos.empty() || !new_image_infos.empty())
	{
		buffer_infos = new_buffer_infos;
		image_infos  = new_image_infos;
	}
	else
	{
		LOGW("Calling reset on Descriptor Set with no new buffer infos and no new image infos.");
	}

	this->write_descriptor_sets.clear();
	this->updated_bindings.clear();

	prepare();
}

void HPPDescriptorSet::prepare()
{
	// We don't want to prepare twice during the life cycle of a Descriptor Set
	if (!write_descriptor_sets.empty())
	{
		LOGW("Trying to prepare a descriptor set that has already been prepared, skipping.");
		return;
	}

	// Iterate over all buffer bindings
	for (auto &binding_it : buffer_infos)
	{
		auto  binding_index   = binding_it.first;
		auto &buffer_bindings = binding_it.second;

		if (auto binding_info = descriptor_set_layout.get_layout_binding(binding_index))
		{
			// Iterate over all binding buffers in array
			for (auto &element_it : buffer_bindings)
			{
				auto &buffer_info = element_it.second;

				size_t uniform_buffer_range_limit = device.get_gpu().get_properties().limits.maxUniformBufferRange;
				size_t storage_buffer_range_limit = device.get_gpu().get_properties().limits.maxStorageBufferRange;

				size_t buffer_range_limit = static_cast<size_t>(buffer_info.range);

				if ((binding_info->descriptorType == vk::DescriptorType::eUniformBuffer || binding_info->descriptorType == vk::DescriptorType::eUniformBufferDynamic) && buffer_range_limit > uniform_buffer_range_limit)
				{
					LOGE("Set {} binding {} cannot be updated: buffer size {} exceeds the uniform buffer range limit {}", descriptor_set_layout.get_index(), binding_index, buffer_info.range, uniform_buffer_range_limit);
					buffer_range_limit = uniform_buffer_range_limit;
				}
				else if ((binding_info->descriptorType == vk::DescriptorType::eStorageBuffer || binding_info->descriptorType == vk::DescriptorType::eStorageBufferDynamic) && buffer_range_limit > storage_buffer_range_limit)
				{
					LOGE("Set {} binding {} cannot be updated: buffer size {} exceeds the storage buffer range limit {}", descriptor_set_layout.get_index(), binding_index, buffer_info.range, storage_buffer_range_limit);
					buffer_range_limit = storage_buffer_range_limit;
				}

				// Clip the buffers range to the limit if one exists as otherwise we will receive a Vulkan validation error
				buffer_info.range = buffer_range_limit;

				write_descriptor_sets.push_back(vk::WriteDescriptorSet{
				    handle,
				    binding_index,
				    element_it.first,
				    binding_info->descriptorType,
				    nullptr,
				    buffer_info,
				});
			}
		}
		else
		{
			LOGE("Shader layout set does not use buffer binding at #{}", binding_index);
		}
	}

	// Iterate over all image bindings
	for (auto &binding_it : image_infos)
	{
		auto  binding_index     = binding_it.first;
		auto &binding_resources = binding_it.second;

		if (auto binding_info = descriptor_set_layout.get_layout_binding(binding_index))
		{
			// Iterate over all binding images in array
			for (auto &element_it : binding_resources)
			{
				write_descriptor_sets.push_back(vk::WriteDescriptorSet{
				    handle,
				    binding_index,
				    element_it.first,
				    binding_info->descriptorType,
				    element_it.second,
				});
			}
		}
		else
		{
			LOGE("Shader layout set does not use image binding at #{}", binding_index);
		}
	}
}

void HPPDescriptorSet::update(const std::vector<uint32_t> &bindings_to_update)
{
	std::vector<vk::WriteDescriptorSet> write_operations;
	std::vector<size_t>                 write_operation_hashes;

	// If the 'bindings_to_update' vector is empty, we want to write to all the bindings
	// (but skipping all to-update bindings that haven't been written yet)
	if (bindings_to_update.empty())
	{
		for (size_t i = 0; i < write_descriptor_sets.size(); i++)
		{
			const auto &write_operation = write_descriptor_sets[i];

			size_t write_operation_hash = 0;
			hash_param(write_operation_hash, write_operation);

			auto update_pair_it = updated_bindings.find(write_operation.dstBinding);
			if (update_pair_it == updated_bindings.end() || update_pair_it->second != write_operation_hash)
			{
				write_operations.push_back(write_operation);
				write_operation_hashes.push_back(write_operation_hash);
			}
		}
	}
	else
	{
		// Otherwise we want to update the binding indices present in the 'bindings_to_update' vector.
		// (again, skipping those to update but not updated yet)
		for (size_t i = 0; i < write_descriptor_sets.size(); i++)
		{
			const auto &write_operation = write_descriptor_sets[i];

			if (std::find(bindings_to_update.begin(), bindings_to_update.end(), write_operation.dstBinding) != bindings_to_update.end())
			{
				size_t write_operation_hash = 0;
				hash_param(write_operation_hash, write_operation);

				auto update_pair_it = updated_bindings.find(write_operation.dstBinding);
				if (update_pair_it == updated_bindings.end() || update_pair_it->second != write_operation_hash)
				{
					write_operations.push_back(write_operation);
					write_operation_hashes.push_back(write_operation_hash);
				}
			}
		}
	}

	// Perform the Vulkan call to update the HPPDescriptorSet by executing the write operations
	if (!write_operations.empty())
	{
		device.get_handle().updateDescriptorSets(write_operations, nullptr);
	}

	// Store the bindings from the write operations that were executed by vkUpdateDescriptorSets (and their hash)
	// to prevent overwriting by future calls to "update()"
	for (size_t i = 0; i < write_operations.size(); i++)
	{
		updated_bindings[write_operations[i].dstBinding] = write_operation_hashes[i];
	}
}

void HPPDescriptorSet::apply_writes() const
{
	device.get_handle().updateDescriptorSets(write_descriptor_sets, nullptr);
}

HPPDescriptorSet::HPPDescriptorSet(HPPDescriptorSet &&other) :
    device{other.device},
    descriptor_set_layout{other.descriptor_set_layout},
    descriptor_pool{other.descriptor_pool},
    buffer_infos{std::move(other.buffer_infos)},
    image_infos{std::move(other.image_infos)},
    handle{other.handle},
    write_descriptor_sets{std::move(other.write_descriptor_sets)},
    updated_bindings{std::move(other.updated_bindings)}
{
	other.handle = VK_NULL_HANDLE;
}

vk::DescriptorSet HPPDescriptorSet::get_handle() const
{
	return handle;
}

const HPPDescriptorSetLayout &HPPDescriptorSet::get_layout() const
{
	return descriptor_set_layout;
}

BindingMap<vk::DescriptorBufferInfo> &HPPDescriptorSet::get_buffer_infos()
{
	return buffer_infos;
}

BindingMap<vk::DescriptorImageInfo> &HPPDescriptorSet::get_image_infos()
{
	return image_infos;
}

}        // namespace core
}        // namespace vkb
