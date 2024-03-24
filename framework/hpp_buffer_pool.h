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

/**
 * @brief An allocation of vulkan memory; different buffer allocations,
 *        with different offset and size, may come from the same Vulkan buffer
 */
class HPPBufferAllocation
{
  public:
	HPPBufferAllocation() = default;

	HPPBufferAllocation(core::HPPBuffer &buffer, vk::DeviceSize size, vk::DeviceSize offset);

	HPPBufferAllocation(const HPPBufferAllocation &) = delete;

	HPPBufferAllocation(HPPBufferAllocation &&) = default;

	HPPBufferAllocation &operator=(const HPPBufferAllocation &) = delete;

	HPPBufferAllocation &operator=(HPPBufferAllocation &&) = default;

	void update(const std::vector<uint8_t> &data, vk::DeviceSize offset = 0);

	template <class T>
	void update(const T &value, uint32_t offset = 0)
	{
		update(to_bytes(value), offset);
	}

	bool empty() const;

	vk::DeviceSize get_size() const;

	vk::DeviceSize get_offset() const;

	core::HPPBuffer &get_buffer();

  private:
	core::HPPBuffer *buffer{nullptr};

	vk::DeviceSize base_offset{0};

	vk::DeviceSize size{0};
};

/**
 * @brief Helper class which handles multiple allocation from the same underlying Vulkan buffer.
 */
class HPPBufferBlock
{
  public:
	HPPBufferBlock(core::HPPDevice &device, vk::DeviceSize size, const vk::BufferUsageFlags & usage, VmaMemoryUsage memory_usage);

	/**
	 * @brief check if this HPPBufferBlock can allocate a given amount of memory
	 * @param size the number of bytes to check
	 * @return \c true if \a size bytes can be allocated from this \c HPPBufferBlock, otherwise \c false.
	 */
	bool can_allocate(vk::DeviceSize size) const;

	/**
	 * @return An usable view on a portion of the underlying buffer
	 */
	HPPBufferAllocation allocate(vk::DeviceSize size);

	vk::DeviceSize get_size() const;

	void reset();

  private:
	/**
	 * @ brief Determine the current aligned offset.
	 * @return The current aligned offset.
	 */
	vk::DeviceSize aligned_offset() const;

  private:
	std::unique_ptr<core::HPPBuffer> buffer;

	// Memory alignment, it may change according to the usage
	vk::DeviceSize alignment{0};

	// Current offset, it increases on every allocation
	vk::DeviceSize offset{0};
};

/**
 * @brief A pool of buffer blocks for a specific usage.
 * It may contain inactive blocks that can be recycled.
 *
 * BufferPool is a linear allocator for buffer chunks, it gives you a view of the size you want.
 * A HPPBufferBlock is the corresponding VkBuffer and you can get smaller offsets inside it.
 * Since a shader cannot specify dynamic UBOs, it has to be done from the code
 * (set_resource_dynamic).
 *
 * When a new frame starts, buffer blocks are returned: the offset is reset and contents are
 * overwritten. The minimum allocation size is 256 kb, if you ask for more you get a dedicated
 * buffer allocation.
 *
 * We re-use descriptor sets: we only need one for the corresponding buffer infos (and we only
 * have one VkBuffer per HPPBufferBlock), then it is bound and we use dynamic offsets.
 */
class HPPBufferPool
{
  public:
	HPPBufferPool(core::HPPDevice &device, vk::DeviceSize block_size, const vk::BufferUsageFlags & usage, VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU);

	HPPBufferBlock &request_buffer_block(vk::DeviceSize minimum_size, bool minimal = false);

	void reset();

  private:
	core::HPPDevice &device;

	/// List of blocks requested
	std::vector<std::unique_ptr<HPPBufferBlock>> buffer_blocks;

	/// Minimum size of the blocks
	vk::DeviceSize block_size{0};

	vk::BufferUsageFlags usage;

	VmaMemoryUsage memory_usage{};
};

}        // namespace vkb
