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
 * @brief Manages an array of fixed size VkDescriptorPool and is able to allocate descriptor sets
 */
class HPPDescriptorPool
{
  public:
	static const uint32_t MAX_SETS_PER_POOL = 16;

	HPPDescriptorPool(HPPDevice                    &device,
	               const HPPDescriptorSetLayout &descriptor_set_layout,
	               uint32_t                   pool_size = MAX_SETS_PER_POOL);

	HPPDescriptorPool(const HPPDescriptorPool &) = delete;

	HPPDescriptorPool(HPPDescriptorPool &&) = default;

	~HPPDescriptorPool();

	HPPDescriptorPool &operator=(const HPPDescriptorPool &) = delete;

	HPPDescriptorPool &operator=(HPPDescriptorPool &&) = delete;

	void reset();

	const HPPDescriptorSetLayout &get_descriptor_set_layout() const;

	void set_descriptor_set_layout(const HPPDescriptorSetLayout &set_layout);

	vk::DescriptorSet allocate();

	vk::Result free(vk::DescriptorSet descriptor_set);

  private:
	HPPDevice &device;

	const HPPDescriptorSetLayout *descriptor_set_layout{nullptr};

	// Descriptor pool size
	std::vector<vk::DescriptorPoolSize> pool_sizes;

	// Number of sets to allocate for each pool
	uint32_t pool_max_sets{0};

	// Total descriptor pools created
	std::vector<vk::DescriptorPool> pools;

	// Count sets for each pool
	std::vector<uint32_t> pool_sets_count;

	// Current pool index to allocate descriptor set
	uint32_t pool_index{0};

	// Map between descriptor set and pool index
	std::unordered_map<vk::DescriptorSet, uint32_t> set_pool_mapping;

	// Find next pool index or create new pool
	uint32_t find_available_pool(uint32_t pool_index);
};
}        // namespace core
}        // namespace vkb
