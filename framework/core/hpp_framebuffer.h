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

#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace rendering
{
class HPPRenderTarget;
}

namespace core
{
class HPPDevice;
class HPPRenderPass;

/**
 * @brief facade class around vkb::Framebuffer, providing a vulkan.hpp-based interface
 *
 * See vkb::Framebuffer for documentation
 */
class HPPFramebuffer
{
  public:
	HPPFramebuffer(HPPDevice &device, const rendering::HPPRenderTarget &render_target, const HPPRenderPass &render_pass);

	HPPFramebuffer(const HPPFramebuffer &) = delete;

	HPPFramebuffer(HPPFramebuffer &&other);

	~HPPFramebuffer();

	HPPFramebuffer &operator=(const HPPFramebuffer &) = delete;

	HPPFramebuffer &operator=(HPPFramebuffer &&) = delete;

	vk::Framebuffer get_handle() const;

	const vk::Extent2D &get_extent() const;

  private:
	HPPDevice &device;

	vk::Framebuffer handle;

	vk::Extent2D extent;
};
}        // namespace core
}        // namespace vkb
