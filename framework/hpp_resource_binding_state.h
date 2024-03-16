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

#include <common/hpp_vk_common.h>
#if 0
#	include "resource_binding_state.h"
#	include <core/hpp_buffer.h>
#	include <core/hpp_image_view.h>
#	include <core/hpp_sampler.h>
#endif

namespace vkb
{
namespace core
{
class HPPBuffer;
class HPPImageView;
class HPPSampler;
class HPPImageView;
}

/**
 * @brief A resource info is a struct containing the actual resource data.
 *
 * This will be referenced by a buffer info or image info descriptor inside a descriptor set.
 */
struct HPPResourceInfo
{
	bool                      dirty      = false;
	const core::HPPBuffer    *buffer     = nullptr;
	vk::DeviceSize            offset     = 0;
	vk::DeviceSize            range      = 0;
	const core::HPPImageView *image_view = nullptr;
	const core::HPPSampler   *sampler    = nullptr;
};
/**
 * @brief A resource set is a set of bindings containing resources that were bound
 *        by a command buffer.
 *
 * The ResourceSet has a one to one mapping with a DescriptorSet.
 */
class HPPResourceSet
{
  public:
	void reset();

	bool is_dirty() const;

	void clear_dirty();

	void clear_dirty(uint32_t binding, uint32_t array_element);

	void bind_buffer(const core::HPPBuffer &buffer, vk::DeviceSize offset, vk::DeviceSize range, uint32_t binding, uint32_t array_element);

	void bind_image(const core::HPPImageView &image_view, const core::HPPSampler &sampler, uint32_t binding, uint32_t array_element);

	void bind_image(const core::HPPImageView &image_view, uint32_t binding, uint32_t array_element);

	void bind_input(const core::HPPImageView &image_view, uint32_t binding, uint32_t array_element);

	const BindingMap<HPPResourceInfo> &get_resource_bindings() const;

  private:
	bool dirty{false};

	BindingMap<HPPResourceInfo> resource_bindings;
};

/**
 * @brief The resource binding state of a command buffer.
 *
 * Keeps track of all the resources bound by the command buffer. The ResourceBindingState is used by
 * the command buffer to create the appropriate descriptor sets when it comes to draw.
 */
class HPPResourceBindingState
{
  public:
	void reset();

	bool is_dirty();

	void clear_dirty();

	void clear_dirty(uint32_t set);

	void bind_buffer(const core::HPPBuffer &buffer, vk::DeviceSize offset, vk::DeviceSize range, uint32_t set, uint32_t binding, uint32_t array_element);

	void bind_image(const core::HPPImageView &image_view, const core::HPPSampler &sampler, uint32_t set, uint32_t binding, uint32_t array_element);

	void bind_image(const core::HPPImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element);

	void bind_input(const core::HPPImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element);

	const std::unordered_map<uint32_t, HPPResourceSet> &get_resource_sets();

  private:
	bool dirty{false};

	std::unordered_map<uint32_t, HPPResourceSet> resource_sets;
};
}        // namespace vkb
