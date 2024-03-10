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

#include "core/swapchain.h"

#include "common/logging.h"
#include "device.h"

namespace vkb
{
namespace
{
inline uint32_t choose_image_count(
    uint32_t request_image_count,
    uint32_t min_image_count,
    uint32_t max_image_count)
{
	if (max_image_count != 0)
	{
		request_image_count = std::min(request_image_count, max_image_count);
	}

	request_image_count = std::max(request_image_count, min_image_count);

	return request_image_count;
}

inline uint32_t choose_image_array_layers(
    uint32_t request_image_array_layers,
    uint32_t max_image_array_layers)
{
	request_image_array_layers = std::min(request_image_array_layers, max_image_array_layers);
	request_image_array_layers = std::max(request_image_array_layers, 1u);

	return request_image_array_layers;
}

inline VkExtent2D choose_extent(
    VkExtent2D        request_extent,
    const VkExtent2D &min_image_extent,
    const VkExtent2D &max_image_extent,
    const VkExtent2D &current_extent)
{
	if (current_extent.width == 0xFFFFFFFF)
	{
		return request_extent;
	}

	if (request_extent.width < 1 || request_extent.height < 1)
	{
		LOGW("(Swapchain) Image extent ({}, {}) not supported. Selecting ({}, {}).", request_extent.width, request_extent.height, current_extent.width, current_extent.height);
		return current_extent;
	}

	request_extent.width = std::max(request_extent.width, min_image_extent.width);
	request_extent.width = std::min(request_extent.width, max_image_extent.width);

	request_extent.height = std::max(request_extent.height, min_image_extent.height);
	request_extent.height = std::min(request_extent.height, max_image_extent.height);

	return request_extent;
}

inline VkPresentModeKHR choose_present_mode(
    const Device                        &device,
    VkSurfaceKHR                         surface,
    VkPresentModeKHR                     request_present_mode,
    const std::vector<VkPresentModeKHR> &present_mode_priority_list)
{
	std::vector<VkPresentModeKHR> available_present_modes;
	uint32_t                      present_mode_count{0U};
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device.get_gpu().get_handle(), surface, &present_mode_count, nullptr));
	available_present_modes.resize(present_mode_count);
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device.get_gpu().get_handle(), surface, &present_mode_count, available_present_modes.data()));
	LOGI("Surface supports the following present modes:");
	for (auto &present_mode : available_present_modes)
	{
		LOGI("  \t{}", to_string(present_mode));
	}

	auto present_mode_it = std::find(available_present_modes.begin(), available_present_modes.end(), request_present_mode);

	if (present_mode_it == available_present_modes.end())
	{
		// If nothing found, always default to FIFO
		VkPresentModeKHR chosen_present_mode = VK_PRESENT_MODE_FIFO_KHR;

		for (auto &present_mode : present_mode_priority_list)
		{
			if (std::find(available_present_modes.begin(), available_present_modes.end(), present_mode) != available_present_modes.end())
			{
				chosen_present_mode = present_mode;
				break;
			}
		}

		LOGW("(Swapchain) Present mode '{}' not supported. Selecting '{}'.", to_string(request_present_mode), to_string(chosen_present_mode));
		return chosen_present_mode;
	}
	else
	{
		LOGI("(Swapchain) Present mode selected: {}", to_string(request_present_mode));
		return *present_mode_it;
	}
}

inline VkSurfaceFormatKHR choose_surface_format(
    const Device                          &device,
    VkSurfaceKHR                           surface,
    const VkSurfaceFormatKHR               requested_surface_format,
    const std::vector<VkSurfaceFormatKHR> &surface_format_priority_list)
{
	uint32_t                        surface_format_count{0U};
	std::vector<VkSurfaceFormatKHR> available_surface_formats;
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device.get_gpu().get_handle(), surface, &surface_format_count, nullptr));
	available_surface_formats.resize(surface_format_count);
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device.get_gpu().get_handle(), surface, &surface_format_count, available_surface_formats.data()));

	LOGI("Surface supports the following surface formats:");
	for (auto &surface_format : available_surface_formats)
	{
		LOGI("  \t{}", to_string(surface_format));
	}

	// Try to find the requested surface format in the supported surface formats
	auto surface_format_it = std::find_if(
	    available_surface_formats.begin(),
	    available_surface_formats.end(),
	    [&requested_surface_format](const VkSurfaceFormatKHR &surface) {
		    if (surface.format == requested_surface_format.format &&
		        surface.colorSpace == requested_surface_format.colorSpace)
		    {
			    return true;
		    }

		    return false;
	    });

	// If the requested surface format isn't found, then try to request a format from the priority list
	if (surface_format_it == available_surface_formats.end())
	{
		for (auto &surface_format : surface_format_priority_list)
		{
			surface_format_it = std::find_if(
			    available_surface_formats.begin(),
			    available_surface_formats.end(),
			    [&surface_format](const VkSurfaceFormatKHR &surface) {
				    if (surface.format == surface_format.format &&
				        surface.colorSpace == surface_format.colorSpace)
				    {
					    return true;
				    }

				    return false;
			    });
			if (surface_format_it != available_surface_formats.end())
			{
				LOGW("(Swapchain) Surface format ({}) not supported. Selecting ({}).", to_string(requested_surface_format), to_string(*surface_format_it));
				return *surface_format_it;
			}
		}

		// If nothing found, default the first supporte surface format
		surface_format_it = available_surface_formats.begin();
		LOGW("(Swapchain) Surface format ({}) not supported. Selecting ({}).", to_string(requested_surface_format), to_string(*surface_format_it));
	}
	else
	{
		LOGI("(Swapchain) Surface format selected: {}", to_string(requested_surface_format));
	}

	return *surface_format_it;
}

inline VkSurfaceTransformFlagBitsKHR choose_transform(
    VkSurfaceTransformFlagBitsKHR request_transform,
    VkSurfaceTransformFlagsKHR    supported_transform,
    VkSurfaceTransformFlagBitsKHR current_transform)
{
	if (request_transform & supported_transform)
	{
		return request_transform;
	}

	LOGW("(Swapchain) Surface transform '{}' not supported. Selecting '{}'.", to_string(request_transform), to_string(current_transform));

	return current_transform;
}

inline VkCompositeAlphaFlagBitsKHR choose_composite_alpha(VkCompositeAlphaFlagBitsKHR request_composite_alpha, VkCompositeAlphaFlagsKHR supported_composite_alpha)
{
	if (request_composite_alpha & supported_composite_alpha)
	{
		return request_composite_alpha;
	}

	static const std::vector<VkCompositeAlphaFlagBitsKHR> composite_alpha_flags = {
	    VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
	    VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
	    VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
	    VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR};

	for (VkCompositeAlphaFlagBitsKHR composite_alpha : composite_alpha_flags)
	{
		if (composite_alpha & supported_composite_alpha)
		{
			LOGW("(Swapchain) Composite alpha '{}' not supported. Selecting '{}.", to_string(request_composite_alpha), to_string(composite_alpha));
			return composite_alpha;
		}
	}

	throw std::runtime_error("No compatible composite alpha found.");
}

inline bool validate_format_feature(VkImageUsageFlagBits image_usage, VkFormatFeatureFlags supported_features)
{
	switch (image_usage)
	{
		case VK_IMAGE_USAGE_STORAGE_BIT:
			return VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT & supported_features;
		default:
			return true;
	}
}

template <typename FlagType, typename BitType>
void for_each_filtered_flag(const FlagType &requested_flags, const std::function<void(BitType)> &f)
{
	using MaskType    = typename std::underlying_type<BitType>::type;
	MaskType test_bit = 1;
	while (test_bit != 0)
	{
		BitType m = static_cast<BitType>(test_bit);
		if (requested_flags & m)
		{
			f(m);
		}
		test_bit <<= 1;
	}
}

inline VkImageUsageFlags choose_image_usage(const VkImageUsageFlags &requested_image_usage_flags, VkImageUsageFlags supported_image_usage, VkFormatFeatureFlags supported_features)
{
	VkImageUsageFlags validated_image_usage_flags = 0;
	for_each_filtered_flag<VkImageUsageFlags, VkImageUsageFlagBits>(requested_image_usage_flags, [&](VkImageUsageFlagBits flag) {
		if ((flag & supported_image_usage) && validate_format_feature(flag, supported_features))
		{
			validated_image_usage_flags |= flag;
		}
		else
		{
			LOGW("(Swapchain) Image usage ({}) requested but not supported.", to_string(flag));
		}
	});

	if (!validated_image_usage_flags)
	{
		// Pick the first format from list of defaults, if supported
		constexpr VkImageUsageFlags default_usage_flags =
		    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
		    VK_IMAGE_USAGE_STORAGE_BIT |
		    VK_IMAGE_USAGE_SAMPLED_BIT |
		    VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		for_each_filtered_flag<VkImageUsageFlags, VkImageUsageFlagBits>(
		    default_usage_flags,
		    [&](VkImageUsageFlagBits flag) {
			    if (!validated_image_usage_flags && (flag & supported_image_usage) && validate_format_feature(flag, supported_features))
			    {
				    validated_image_usage_flags |= flag;
			    }
		    });
	}

	if (validated_image_usage_flags)
	{
		// Log image usage flags used
		std::string usage_list;
		for_each_filtered_flag<VkImageUsageFlags, VkImageUsageFlagBits>(
		    validated_image_usage_flags,
		    [&](VkImageUsageFlagBits image_usage) {
			    usage_list += to_string(image_usage) + " ";
		    });
		LOGI("(Swapchain) Image usage flags: {}", usage_list);
	}
	else
	{
		throw std::runtime_error("No compatible image usage found.");
	}

	return validated_image_usage_flags;
}

}        // namespace

Swapchain::Swapchain(const Swapchain &old_swapchain, const VkExtent2D &extent) :
    Swapchain(old_swapchain)
{
	properties.extent = extent;
	init();
}

Swapchain::Swapchain(const Swapchain &old_swapchain, const uint32_t image_count) :
    Swapchain(old_swapchain)
{
	properties.image_count = image_count;
	init();
}

Swapchain::Swapchain(const Swapchain &old_swapchain, const VkImageUsageFlags &image_usage_flags, bool _) :
    Swapchain(old_swapchain)
{
	properties.image_usage = image_usage_flags;
	init();
}

Swapchain::Swapchain(const Swapchain &old_swapchain, const VkExtent2D &extent, const VkSurfaceTransformFlagBitsKHR transform) :
    Swapchain(old_swapchain)
{
	properties.extent        = extent;
	properties.pre_transform = transform;
	init();
}

Swapchain::Swapchain(const Swapchain &old_swapchain) :
    device{old_swapchain.device},
    surface{old_swapchain.surface},
    properties{old_swapchain.properties},
    present_mode_priority_list{old_swapchain.present_mode_priority_list},
    surface_format_priority_list{old_swapchain.surface_format_priority_list}
{
	properties.old_swapchain = old_swapchain.handle;
}

Swapchain::Swapchain(Device                                &device,
                     VkSurfaceKHR                           surface,
                     const VkPresentModeKHR                 present_mode,
                     std::vector<VkPresentModeKHR> const   &present_mode_priority_list,
                     const std::vector<VkSurfaceFormatKHR> &surface_format_priority_list,
                     const VkExtent2D                      &extent,
                     const uint32_t                         image_count,
                     const VkSurfaceTransformFlagBitsKHR    transform,
                     const VkImageUsageFlags               &image_usage_flags,
                     VkSwapchainKHR                         old_swapchain) :
    device{device},
    surface{surface},
    present_mode_priority_list{present_mode_priority_list},
    surface_format_priority_list{surface_format_priority_list},
    properties{
        old_swapchain,
        image_count,
        extent,
        {},
        1,
        image_usage_flags,
        transform,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
        VK_PRESENT_MODE_IMMEDIATE_KHR,
    }
{
	init();
}

void Swapchain::init()
{
	VkSurfaceCapabilitiesKHR surface_capabilities{};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->device.get_gpu().get_handle(), surface, &surface_capabilities);

	// Chose best properties based on surface capabilities
	properties.image_count    = choose_image_count(properties.image_count, surface_capabilities.minImageCount, surface_capabilities.maxImageCount);
	properties.extent         = choose_extent(properties.extent, surface_capabilities.minImageExtent, surface_capabilities.maxImageExtent, surface_capabilities.currentExtent);
	properties.array_layers   = choose_image_array_layers(1U, surface_capabilities.maxImageArrayLayers);
	properties.surface_format = choose_surface_format(this->device, surface, properties.surface_format, surface_format_priority_list);
	VkFormatProperties format_properties;
	vkGetPhysicalDeviceFormatProperties(this->device.get_gpu().get_handle(), properties.surface_format.format, &format_properties);
	properties.image_usage     = choose_image_usage(properties.image_usage, surface_capabilities.supportedUsageFlags, format_properties.optimalTilingFeatures);
	properties.pre_transform   = choose_transform(properties.pre_transform, surface_capabilities.supportedTransforms, surface_capabilities.currentTransform);
	properties.composite_alpha = choose_composite_alpha(VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR, surface_capabilities.supportedCompositeAlpha);

	// Pass through defaults to the create function
	properties.present_mode = choose_present_mode(this->device, surface, properties.present_mode, present_mode_priority_list);

	VkSwapchainCreateInfoKHR create_info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
	create_info.minImageCount    = properties.image_count;
	create_info.imageExtent      = properties.extent;
	create_info.presentMode      = properties.present_mode;
	create_info.imageFormat      = properties.surface_format.format;
	create_info.imageColorSpace  = properties.surface_format.colorSpace;
	create_info.imageArrayLayers = properties.array_layers;
	create_info.imageUsage       = properties.image_usage;
	create_info.preTransform     = properties.pre_transform;
	create_info.compositeAlpha   = properties.composite_alpha;
	create_info.oldSwapchain     = properties.old_swapchain;
	create_info.surface          = surface;

	VkResult result = vkCreateSwapchainKHR(device.get_handle(), &create_info, nullptr, &handle);

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Cannot create Swapchain"};
	}

	uint32_t image_available{0u};
	VK_CHECK(vkGetSwapchainImagesKHR(device.get_handle(), handle, &image_available, nullptr));

	images.resize(image_available);

	VK_CHECK(vkGetSwapchainImagesKHR(device.get_handle(), handle, &image_available, images.data()));
}

Swapchain::~Swapchain()
{
	if (handle != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(device.get_handle(), handle, nullptr);
	}
}

Swapchain::Swapchain(Swapchain &&other) noexcept :
    device{other.device},
    surface{std::exchange(other.surface, VK_NULL_HANDLE)},
    handle{std::exchange(other.handle, VK_NULL_HANDLE)},
    images{std::exchange(other.images, {})},
    properties{std::exchange(other.properties, {})},
    present_mode_priority_list{std::exchange(other.present_mode_priority_list, {})},
    surface_format_priority_list{std::exchange(other.surface_format_priority_list, {})}
{
}

bool Swapchain::is_valid() const
{
	return handle != VK_NULL_HANDLE;
}

Device &Swapchain::get_device()
{
	return device;
}

VkSwapchainKHR Swapchain::get_handle() const
{
	return handle;
}

VkResult Swapchain::acquire_next_image(uint32_t &image_index, VkSemaphore image_acquired_semaphore, VkFence fence) const
{
	return vkAcquireNextImageKHR(device.get_handle(), handle, std::numeric_limits<uint64_t>::max(), image_acquired_semaphore, fence, &image_index);
}

const VkExtent2D &Swapchain::get_extent() const
{
	return properties.extent;
}

VkFormat Swapchain::get_format() const
{
	return properties.surface_format.format;
}

const std::vector<VkImage> &Swapchain::get_images() const
{
	return images;
}

VkSurfaceTransformFlagBitsKHR Swapchain::get_transform() const
{
	return properties.pre_transform;
}

VkSurfaceKHR Swapchain::get_surface() const
{
	return surface;
}

VkImageUsageFlags Swapchain::get_usage() const
{
	return properties.image_usage;
}

VkPresentModeKHR Swapchain::get_present_mode() const
{
	return properties.present_mode;
}
}        // namespace vkb
