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

#include "hpp_buffer_pool.h"
#include "core/hpp_buffer.h"
#include "core/hpp_device.h"

namespace vkb
{
HPPBufferBlock::HPPBufferBlock(core::HPPDevice &device, vk::DeviceSize size, const vk::BufferUsageFlags& usage, VmaMemoryUsage memory_usage)
{
	buffer = core::HPPBufferBuilder(size).with_usage(usage).with_vma_usage(memory_usage).build_unique(device);
	if (usage & vk::BufferUsageFlagBits::eUniformBuffer)
	{
		alignment = device.get_gpu().get_properties().limits.minUniformBufferOffsetAlignment;
	}
	else if (usage & vk::BufferUsageFlagBits::eStorageBuffer)
	{
		alignment = device.get_gpu().get_properties().limits.minStorageBufferOffsetAlignment;
	}
	else if (usage & vk::BufferUsageFlagBits::eUniformTexelBuffer)
	{
		alignment = device.get_gpu().get_properties().limits.minTexelBufferOffsetAlignment;
	}
	else if (usage & vk::BufferUsageFlagBits::eIndexBuffer || usage & vk::BufferUsageFlagBits::eVertexBuffer || usage & vk::BufferUsageFlagBits::eIndirectBuffer)
	{
		// Used to calculate the offset, required when allocating memory (its value should be power of 2)
		alignment = 16;
	}
	else
	{
		throw std::runtime_error("Usage not recognised");
	}
}

vk::DeviceSize HPPBufferBlock::aligned_offset() const
{
	return (offset + alignment - 1) & ~(alignment - 1);
}

bool HPPBufferBlock::can_allocate(vk::DeviceSize size) const
{
	assert(size > 0 && "Allocation size must be greater than zero");
	return (aligned_offset() + size <= buffer->get_size());
}

HPPBufferAllocation HPPBufferBlock::allocate(vk::DeviceSize size)
{
	if (can_allocate(size))
	{
		// Move the current offset and return an allocation
		auto aligned = aligned_offset();
		offset       = aligned + size;
		return HPPBufferAllocation{*buffer, size, aligned};
	}

	// No more space available from the underlying buffer, return empty allocation
	return HPPBufferAllocation{};
}

vk::DeviceSize HPPBufferBlock::get_size() const
{
	return buffer->get_size();
}

void HPPBufferBlock::reset()
{
	offset = 0;
}

HPPBufferPool::HPPBufferPool(core::HPPDevice &device, vk::DeviceSize block_size, const vk::BufferUsageFlags& usage, VmaMemoryUsage memory_usage) :
    device{device},
    block_size{block_size},
    usage{usage},
    memory_usage{memory_usage}
{
}

HPPBufferBlock &HPPBufferPool::request_buffer_block(const vk::DeviceSize minimum_size, bool minimal)
{
	// Find a block in the range of the blocks which can fit the minimum size
	auto it = minimal ? std::find_if(buffer_blocks.begin(),
	                                 buffer_blocks.end(),
	                                 [&minimum_size](const std::unique_ptr<HPPBufferBlock> &buffer_block) { return (buffer_block->get_size() == minimum_size) && buffer_block->can_allocate(minimum_size); }) :
	                    std::find_if(buffer_blocks.begin(),
	                                 buffer_blocks.end(),
	                                 [&minimum_size](const std::unique_ptr<HPPBufferBlock> &buffer_block) { return buffer_block->can_allocate(minimum_size); });

	if (it == buffer_blocks.end())
	{
		LOGD("Building #{} buffer block ({})", buffer_blocks.size(), usage);

		vk::DeviceSize new_block_size = minimal ? minimum_size : std::max(block_size, minimum_size);

		// Create a new block and get the iterator on it
		it = buffer_blocks.emplace(buffer_blocks.end(), std::make_unique<HPPBufferBlock>(device, new_block_size, usage, memory_usage));
	}

	return *it->get();
}

void HPPBufferPool::reset()
{
	for (auto &buffer_block : buffer_blocks)
	{
		buffer_block->reset();
	}
}

HPPBufferAllocation::HPPBufferAllocation(core::HPPBuffer &buffer, vk::DeviceSize size, vk::DeviceSize offset) :
    buffer{&buffer},
    size{size},
    base_offset{offset}
{
}

void HPPBufferAllocation::update(const std::vector<uint8_t> &data, vk::DeviceSize offset)
{
	assert(buffer && "Invalid buffer pointer");

	if (offset + data.size() <= size)
	{
		buffer->update(vk::ArrayProxy<const uint8_t>{data}, base_offset + offset);
	}
	else
	{
		LOGE("Ignore buffer allocation update");
	}
}

bool HPPBufferAllocation::empty() const
{
	return size == 0 || buffer == nullptr;
}

vk::DeviceSize HPPBufferAllocation::get_size() const
{
	return size;
}

vk::DeviceSize HPPBufferAllocation::get_offset() const
{
	return base_offset;
}

core::HPPBuffer &HPPBufferAllocation::get_buffer()
{
	assert(buffer && "Invalid buffer pointer");
	return *buffer;
}

}        // namespace vkb
