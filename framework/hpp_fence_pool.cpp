/* Copyright (c) 2019-2023, Arm Limited and Contributors
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

#include "hpp_fence_pool.h"

#include "core/hpp_device.h"

namespace vkb
{
HPPFencePool::HPPFencePool(core::HPPDevice &device) :
    device{device}
{
}

HPPFencePool::~HPPFencePool()
{
	auto result = wait();
	if (result != vk::Result::eSuccess)
	{
		LOGW("Failed to wait for all fences to be signaled: {}", vk::to_string(result));
	}
	result = reset();
	if (result != vk::Result::eSuccess)
	{
		LOGW("Failed to reset all fences: {}", vk::to_string(result));
	}

	// Destroy all fences
	for (vk::Fence fence : fences)
	{
		device.get_handle().destroy(fence);
	}
	fences.clear();
}

vk::Fence HPPFencePool::request_fence()
{
	// Check if there is an available fence
	if (active_fence_count < fences.size())
	{
		return fences[active_fence_count++];
	}

	auto fence = device.get_handle().createFence(vk::FenceCreateInfo{});

	fences.push_back(fence);

	active_fence_count++;

	return fences.back();
}

[[nodiscard]] vk::Result HPPFencePool::wait(uint32_t timeout) const
{
	if (active_fence_count < 1 || fences.empty())
	{
		return vk::Result::eSuccess;
	}

	return device.get_handle().waitForFences(active_fence_count, fences.data(), true, timeout);
}

[[nodiscard]] vk::Result HPPFencePool::reset()
{
	if (active_fence_count < 1 || fences.empty())
	{
		return vk::Result::eSuccess;
	}

	vk::Result result = device.get_handle().resetFences(active_fence_count, fences.data());

	if (result != vk::Result::eSuccess)
	{
		return result;
	}

	active_fence_count = 0;

	return result;
}
}        // namespace vkb
