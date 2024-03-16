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

#pragma once

#include <common/hpp_error.h>
#include "common/hpp_vk_common.h"
#include <core/hpp_vulkan_resource.h>
#include <vector>
#include <vk_mem_alloc.h>

namespace vkb
{
namespace core
{
class HPPDevice;

}

namespace allocated
{

VmaAllocator &get_memory_allocator();

void init(const vkb::core::HPPDevice &device);
void shutdown();

template <
    typename BuilderType,
    typename CreateInfoType>
struct HPPBuilder
{
	VmaAllocationCreateInfo alloc_create_info{};
	std::string             debug_name;
	CreateInfoType          create_info;

  protected:
	HPPBuilder(const HPPBuilder &other) = delete;

	HPPBuilder(const CreateInfoType &create_info) :
	    create_info(create_info)
	{
		alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
	}

  public:
	BuilderType &with_debug_name(const std::string &name)
	{
		debug_name = name;
		return *static_cast<BuilderType *>(this);
	}

	BuilderType &with_vma_usage(VmaMemoryUsage usage)
	{
		alloc_create_info.usage = usage;
		return *static_cast<BuilderType *>(this);
	}

	BuilderType &with_vma_flags(VmaAllocationCreateFlags flags)
	{
		alloc_create_info.flags = flags;
		return *static_cast<BuilderType *>(this);
	}

	BuilderType &with_vma_required_flags(vk::MemoryPropertyFlags flags)
	{
		alloc_create_info.requiredFlags = flags;
		return *static_cast<BuilderType *>(this);
	}

	BuilderType &with_vma_preferred_flags(vk::MemoryPropertyFlags flags)
	{
		alloc_create_info.preferredFlags = flags;
		return *static_cast<BuilderType *>(this);
	}

	BuilderType &with_memory_type_bits(uint32_t type_bits)
	{
		alloc_create_info.memoryTypeBits = type_bits;
		return *static_cast<BuilderType *>(this);
	}

	BuilderType &with_vma_pool(VmaPool pool)
	{
		alloc_create_info.pool = pool;
		return *static_cast<BuilderType *>(this);
	}

	BuilderType &with_queue_families(uint32_t count, const uint32_t *family_indices)
	{
		create_info.queueFamilyIndexCount = count;
		create_info.pQueueFamilyIndices   = family_indices;
		return *static_cast<BuilderType *>(this);
	}

	BuilderType &with_sharing(VkSharingMode sharing)
	{
		create_info.sharingMode = sharing;
		return *static_cast<BuilderType *>(this);
	}

	BuilderType &with_implicit_sharing_mode()
	{
		if (create_info.queueFamilyIndexCount != 0)
		{
			create_info.sharingMode = vk::SharingMode::eConcurrent;
		}
		else
		{
			create_info.sharingMode = vk::SharingMode::eExclusive;
		}
		return *static_cast<BuilderType *>(this);
	}

	BuilderType &with_queue_families(const vk::ArrayProxyNoTemporaries<const uint32_t> &queue_families)
	{
		return with_queue_families(static_cast<uint32_t>(queue_families.size()), queue_families.data());
	}
};

class HPPAllocatedBase
{
  public:
	HPPAllocatedBase() = default;
	HPPAllocatedBase(const VmaAllocationCreateInfo &alloc_create_info);
	HPPAllocatedBase(HPPAllocatedBase &&other) noexcept;

	HPPAllocatedBase &operator=(const HPPAllocatedBase &) = delete;
	HPPAllocatedBase &operator=(HPPAllocatedBase &&)      = delete;

	const uint8_t *get_data() const;
	VkDeviceMemory get_memory() const;

	/**
	 * @brief Flushes memory if it is HOST_VISIBLE and not HOST_COHERENT
	 */
	void flush(vk::DeviceSize offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE);

	/**
	 * @brief Returns true if the memory is mapped, false otherwise
	 * @return mapping status
	 */
	bool mapped() const;

	/**
	 * @brief Maps vulkan memory if it isn't already mapped to an host visible address
	 * @return Pointer to host visible memory
	 */
	uint8_t *map();

	/**
	 * @brief Unmaps vulkan memory from the host visible address
	 */
	void unmap();

	/**
	 * @brief Copies byte data into the buffer
	 * @param data The data to copy from
	 * @param size The amount of bytes to copy
	 * @param offset The offset to start the copying into the mapped data
	 */
	vk::DeviceSize update(const uint8_t *data, vk::DeviceSize size, vk::DeviceSize offset = 0);

	/**
	 * @brief Converts any non byte data into bytes and then updates the buffer
	 * @param data The data to copy from
	 * @param size The amount of bytes to copy
	 * @param offset The offset to start the copying into the mapped data
	 */
	vk::DeviceSize update(void const *data, vk::DeviceSize size, vk::DeviceSize offset = 0);

	/**
	 * @todo Use the vk::ArrayBuffer class to collapse some of these templates
	 * @brief Copies a vector of items into the buffer
	 * @param data The data vector to upload
	 * @param offset The offset to start the copying into the mapped data
	 */
	template <typename T>
	vk::DeviceSize update(vk::ArrayProxy<const T> const &data, vk::DeviceSize offset = 0)
	{
		return update(data.data(), data.size() * sizeof(T), offset);
	}

	/**
	 * @brief Copies an object as byte data into the buffer
	 * @param object The object to convert into byte data
	 * @param offset The offset to start the copying into the mapped data
	 */
	template <class T>
	vk::DeviceSize convert_and_update(const T &object, vk::DeviceSize offset = 0)
	{
		return update(reinterpret_cast<const uint8_t *>(&object), sizeof(T), offset);
	}

  protected:
	virtual void             post_create(VmaAllocationInfo const &allocation_info);
	[[nodiscard]] vk::Buffer create_buffer(vk::BufferCreateInfo const &create_info);
	[[nodiscard]] vk::Image  create_image(vk::ImageCreateInfo const &create_info);
	void                     destroy_buffer(vk::Buffer buffer);
	void                     destroy_image(vk::Image image);
	void                     clear();

	VmaAllocationCreateInfo alloc_create_info{};
	VmaAllocation           allocation  = VK_NULL_HANDLE;
	uint8_t                *mapped_data = nullptr;
	bool                    coherent    = false;
	bool                    persistent  = false;        // Whether the buffer is persistently mapped or not
};

template <typename HandleType>
class HPPAllocated : public vkb::core::HPPVulkanResource<HandleType>, public HPPAllocatedBase
{
	using ParentType = vkb::core::HPPVulkanResource<HandleType>;

  public:
	using ParentType::get_device;
	using HPPAllocatedBase::update;

	HPPAllocated()               = delete;
	HPPAllocated(const HPPAllocated &) = delete;

	// Import the base class constructors
	template <typename... Args>
	HPPAllocated(const VmaAllocationCreateInfo &alloc_create_info, Args &&...args) :
	    ParentType(std::forward<Args>(args)...),
	    HPPAllocatedBase(alloc_create_info)
	{
	}

	HPPAllocated(HPPAllocated &&other) noexcept
	    :
	    ParentType{static_cast<ParentType &&>(other)},
	    HPPAllocatedBase{static_cast<HPPAllocatedBase &&>(other)}
	{
	}

	#if 0
	const HandleType *get() const
	{
		return &ParentType::get_handle();
	}
	#endif

	/**
	 * @brief Copies byte data into the buffer
	 * @param data The data to copy from
	 * @param count The number of array elements
	 * @param offset The offset to start the copying into the mapped data
	 */
	template <typename T>
	vk::DeviceSize update_from_array(const T *data, size_t count, size_t offset = 0)
	{
		return update(vk::ArrayProxy<const T>{data, count}, offset);
	}
};

}        // namespace allocated
}        // namespace vkb
