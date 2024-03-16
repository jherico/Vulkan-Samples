/* Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
 * Copyright (c) 2024, Bradley Austin Davis. All rights reserved.
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

#include <core/hpp_allocated.h>

#include <core/hpp_device.h>

namespace vkb
{

namespace allocated
{

void init(const vkb::core::HPPDevice &device)
{
	VmaVulkanFunctions vma_vulkan_func{};
	vma_vulkan_func.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vma_vulkan_func.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo allocator_info{};
	allocator_info.pVulkanFunctions = &vma_vulkan_func;
	allocator_info.physicalDevice   = static_cast<VkPhysicalDevice>(device.get_gpu().get_handle());
	allocator_info.device           = static_cast<VkDevice>(device.get_handle());
	allocator_info.instance         = static_cast<VkInstance>(device.get_gpu().get_instance().get_handle());

	bool can_get_memory_requirements = device.is_extension_supported(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
	bool has_dedicated_allocation    = device.is_extension_supported(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
	if (can_get_memory_requirements && has_dedicated_allocation)
	{
		allocator_info.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
	}

	if (device.is_extension_supported(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) && device.is_enabled(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
	{
		allocator_info.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	}

	if (device.is_extension_supported(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME) && device.is_enabled(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME))
	{
		allocator_info.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
	}

	if (device.is_extension_supported(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME) && device.is_enabled(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME))
	{
		allocator_info.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;
	}

	if (device.is_extension_supported(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME) && device.is_enabled(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME))
	{
		allocator_info.flags |= VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;
	}

	if (device.is_extension_supported(VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME) && device.is_enabled(VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME))
	{
		allocator_info.flags |= VMA_ALLOCATOR_CREATE_AMD_DEVICE_COHERENT_MEMORY_BIT;
	}

	VkResult result = vmaCreateAllocator(&allocator_info, &get_memory_allocator());
	VK_CHECK(result);
}

VmaAllocator &get_memory_allocator()
{
	static VmaAllocator memory_allocator = VK_NULL_HANDLE;
	return memory_allocator;
}

void shutdown()
{
	auto &allocator = get_memory_allocator();
	if (allocator != VK_NULL_HANDLE)
	{
		VmaTotalStatistics stats;
		vmaCalculateStatistics(allocator, &stats);
		LOGI("Total device memory leaked: {} bytes.", stats.total.statistics.allocationBytes);
		vmaDestroyAllocator(allocator);
		allocator = VK_NULL_HANDLE;
	}
}

HPPAllocatedBase::HPPAllocatedBase(const VmaAllocationCreateInfo &alloc_create_info) :
    alloc_create_info(alloc_create_info)
{
}

HPPAllocatedBase::HPPAllocatedBase(HPPAllocatedBase &&other) noexcept :
    alloc_create_info(std::exchange(other.alloc_create_info, {})),
    allocation(std::exchange(other.allocation, {})),
    mapped_data(std::exchange(other.mapped_data, {})),
    coherent(std::exchange(other.coherent, {})),
    persistent(std::exchange(other.persistent, {}))
{
}

const uint8_t *HPPAllocatedBase::get_data() const
{
	return mapped_data;
}

VkDeviceMemory HPPAllocatedBase::get_memory() const
{
	VmaAllocationInfo alloc_info;
	vmaGetAllocationInfo(get_memory_allocator(), allocation, &alloc_info);
	return alloc_info.deviceMemory;
}

void HPPAllocatedBase::flush(vk::DeviceSize offset, vk::DeviceSize size)
{
	if (!coherent)
	{
		vmaFlushAllocation(get_memory_allocator(), allocation, offset, size);
	}
}

uint8_t *HPPAllocatedBase::map()
{
	if (!persistent && !mapped())
	{
		VK_CHECK(vmaMapMemory(get_memory_allocator(), allocation, reinterpret_cast<void **>(&mapped_data)));
		assert(mapped_data);
	}
	return mapped_data;
}

void HPPAllocatedBase::unmap()
{
	if (!persistent && mapped())
	{
		vmaUnmapMemory(get_memory_allocator(), allocation);
		mapped_data = nullptr;
	}
}

vk::DeviceSize HPPAllocatedBase::update(const uint8_t *data, vk::DeviceSize size, vk::DeviceSize offset)
{
	if (persistent)
	{
		std::copy(data, data + size, mapped_data + offset);
		flush();
	}
	else
	{
		map();
		std::copy(data, data + size, mapped_data + offset);
		flush();
		unmap();
	}
	return size;
}

vk::DeviceSize HPPAllocatedBase::update(void const *data, vk::DeviceSize size, vk::DeviceSize offset)
{
	return update(reinterpret_cast<const uint8_t *>(data), size, offset);
}

void HPPAllocatedBase::post_create(VmaAllocationInfo const &allocation_info)
{
	VkMemoryPropertyFlags memory_properties;
	vmaGetAllocationMemoryProperties(get_memory_allocator(), allocation, &memory_properties);
	coherent    = (memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	mapped_data = static_cast<uint8_t *>(allocation_info.pMappedData);
	persistent  = mapped();
}

[[nodiscard]] vk::Buffer HPPAllocatedBase::create_buffer(vk::BufferCreateInfo const &create_info)
{
	VkBuffer          handleResult = VK_NULL_HANDLE;
	VmaAllocationInfo allocation_info{};

	auto result = vmaCreateBuffer(
	    get_memory_allocator(),
	    &(create_info.operator const VkBufferCreateInfo &()),
	    &alloc_create_info,
	    &handleResult,
	    &allocation,
	    &allocation_info);

	VK_CHECK(result);
	post_create(allocation_info);
	return handleResult;
}

[[nodiscard]] vk::Image HPPAllocatedBase::create_image(vk::ImageCreateInfo const &create_info)
{
	assert(0 < create_info.mipLevels && "Images should have at least one level");
	assert(0 < create_info.arrayLayers && "Images should have at least one layer");
	assert(0 < create_info.usage.operator VkImageUsageFlags() && "Images should have at least one usage type");

	VkImage           handleResult = VK_NULL_HANDLE;
	VmaAllocationInfo allocation_info{};

#if 0
		// If the image is an attachment, prefer dedicated memory
		constexpr VkImageUsageFlags attachment_only_flags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
		if (create_info.usage & attachment_only_flags)
		{
			alloc_create_info.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		}

		if (create_info.usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
		{
			alloc_create_info.preferredFlags |= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
		}
#endif

	auto result = vmaCreateImage(
	    get_memory_allocator(),
	    &(create_info.operator const VkImageCreateInfo &()),
	    &alloc_create_info,
	    &handleResult,
	    &allocation,
	    &allocation_info);

	VK_CHECK(result);
	post_create(allocation_info);
	return handleResult;
}

void HPPAllocatedBase::destroy_buffer(vk::Buffer handle)
{
	if (handle && allocation != VK_NULL_HANDLE)
	{
		unmap();
		vmaDestroyBuffer(get_memory_allocator(), handle, allocation);
		clear();
	}
}

void HPPAllocatedBase::destroy_image(vk::Image image)
{
	if (image && allocation != VK_NULL_HANDLE)
	{
		unmap();
		vmaDestroyImage(get_memory_allocator(), image, allocation);
		clear();
	}
}

bool HPPAllocatedBase::mapped() const
{
	return mapped_data != nullptr;
}

void HPPAllocatedBase::clear()
{
	mapped_data       = nullptr;
	persistent        = false;
	alloc_create_info = {};
}

}        // namespace allocated
}        // namespace vkb
