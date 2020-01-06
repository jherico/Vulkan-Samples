/* Copyright (c) 2019, Arm Limited and Contributors
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

#include "rendering/render_target.h"

#include "core/device.h"

namespace vkb
{
namespace
{
struct CompareExtent2D
{
	bool operator()(const vk::Extent2D &lhs, const vk::Extent2D &rhs) const
	{
		return !(lhs.width == rhs.width && lhs.height == rhs.height) && (lhs.width < rhs.width && lhs.height < rhs.height);
	}
};
}        // namespace

Attachment::Attachment(vk::Format format, vk::SampleCountFlagBits samples, vk::ImageUsageFlags usage) :
    format{format},
    samples{samples},
    usage{usage}
{
}
const RenderTarget::CreateFunc RenderTarget::DEFAULT_CREATE_FUNC = [](core::Image &&swapchain_image) -> RenderTarget {
	vk::Format depth_format = get_supported_depth_format(swapchain_image.get_device().get_physical_device());

	core::Image depth_image{swapchain_image.get_device(), swapchain_image.get_extent(),
	                        depth_format,
	                        vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
	                        vma::MemoryUsage::eGpuOnly};

	std::vector<core::Image> images;
	images.push_back(std::move(swapchain_image));
	images.push_back(std::move(depth_image));

	return RenderTarget{std::move(images)};
};

RenderTarget &RenderTarget::operator=(RenderTarget &&other) noexcept
{
	if (this != &other)
	{
		assert(&device == &other.device && "Cannot move assign with a render target created with a different device");

		// Update those descriptor sets referring to old views
		for (size_t i = 0; i < views.size(); ++i)
		{
			device.get_resource_cache().update_descriptor_sets(views, other.views);
		}

		std::swap(extent, other.extent);
		std::swap(images, other.images);
		std::swap(views, other.views);
		std::swap(attachments, other.attachments);
		std::swap(output_attachments, other.output_attachments);
	}
	return *this;
}

vkb::RenderTarget::RenderTarget(std::vector<core::Image> &&images) :
    device{images.back().get_device()},
    images{std::move(images)}
{
	assert(!this->images.empty() && "Should specify at least 1 image");

	std::set<vk::Extent2D, CompareExtent2D> unique_extent;

	// Returns the image extent as a vk::Extent2D structure from a vk::Extent3D
	auto get_image_extent = [](const core::Image &image) { return vk::Extent2D{image.get_extent().width, image.get_extent().height}; };

	// Constructs a set of unique image extens given a vector of images
	std::transform(this->images.begin(), this->images.end(), std::inserter(unique_extent, unique_extent.end()), get_image_extent);

	// Allow only one extent size for a render target
	if (unique_extent.size() != 1)
	{
		vk::throwResultException(vk::Result::eErrorInitializationFailed, "Extent size is not unique");
	}

	extent = *unique_extent.begin();

	for (auto &image : this->images)
	{
		if (image.get_type() != vk::ImageType::e2D)
		{
			vk::throwResultException(vk::Result::eErrorInitializationFailed, "Image type is not 2D");
		}

		views.emplace_back(image, vk::ImageViewType::e2D);

		attachments.emplace_back(Attachment{image.get_format(), image.get_sample_count(), image.get_usage()});
	}
}

const vk::Extent2D &RenderTarget::get_extent() const
{
	return extent;
}

const std::vector<core::ImageView> &RenderTarget::get_views() const
{
	return views;
}

const std::vector<Attachment> &RenderTarget::get_attachments() const
{
	return attachments;
}

void RenderTarget::set_input_attachments(std::vector<uint32_t> &input)
{
	input_attachments = input;
}

const std::vector<uint32_t> &RenderTarget::get_input_attachments() const
{
	return input_attachments;
}

void RenderTarget::set_output_attachments(std::vector<uint32_t> &output)
{
	output_attachments = output;
}

const std::vector<uint32_t> &RenderTarget::get_output_attachments() const
{
	return output_attachments;
}

}        // namespace vkb
