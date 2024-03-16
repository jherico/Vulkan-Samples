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

#include <core/hpp_vulkan_resource.h>

namespace vkb
{
namespace common
{
struct HPPLoadStoreInfo;

}
namespace rendering
{
struct HPPAttachment;
}

namespace core
{
/**
 * @brief facade class around vkb::RenderPass, providing a vulkan.hpp-based interface
 *
 * See vkb::RenderPass for documentation
 */

struct HPPSubpassInfo
{
	std::vector<uint32_t>   input_attachments;
	std::vector<uint32_t>   output_attachments;
	std::vector<uint32_t>   color_resolve_attachments;
	bool                    disable_depth_stencil_attachment;
	uint32_t                depth_stencil_resolve_attachment;
	vk::ResolveModeFlagBits depth_stencil_resolve_mode;
	std::string             debug_name;
};

class HPPRenderPass : public HPPVulkanResource<vk::RenderPass>
{
	HPPRenderPass(
	    HPPDevice                           &device,
	    const std::vector<vkb::rendering::HPPAttachment> &attachments,
	    const std::vector<vkb::common::HPPLoadStoreInfo> &load_store_infos,
	    const std::vector<HPPSubpassInfo>   &subpasses);

	HPPRenderPass(const HPPRenderPass &) = delete;

	HPPRenderPass(HPPRenderPass &&other);

	~HPPRenderPass();

	HPPRenderPass &operator=(const HPPRenderPass &) = delete;

	HPPRenderPass &operator=(HPPRenderPass &&) = delete;

	const uint32_t get_color_output_count(uint32_t subpass_index) const;

	const VkExtent2D get_render_area_granularity() const;

  private:
	size_t subpass_count;

	template <typename T_SubpassDescription, typename T_AttachmentDescription, typename T_AttachmentReference, typename T_SubpassDependency, typename T_RenderPassCreateInfo>
	void create_renderpass(const std::vector<vkb::rendering::HPPAttachment> &attachments, const std::vector<vkb::common::HPPLoadStoreInfo> &load_store_infos, const std::vector<HPPSubpassInfo> &subpasses);

	std::vector<uint32_t> color_output_count;
};
}        // namespace core
}        // namespace vkb

