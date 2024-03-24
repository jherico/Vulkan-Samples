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

#include "hpp_semaphore_pool.h"
#include "core/hpp_device.h"

namespace vkb
{
HPPSemaphorePool::HPPSemaphorePool(core::HPPDevice &device) :
    device{device}
{
}

HPPSemaphorePool::~HPPSemaphorePool()
{
	reset();

	// Destroy all semaphores
	for (vk::Semaphore semaphore : semaphores)
	{
		vkDestroySemaphore(device.get_handle(), semaphore, nullptr);
	}

	semaphores.clear();
}

vk::Semaphore HPPSemaphorePool::request_semaphore_with_ownership()
{
	// Check if there is an available semaphore, if so, just pilfer one.
	if (active_semaphore_count < semaphores.size())
	{
		vk::Semaphore semaphore = semaphores.back();
		semaphores.pop_back();
		return semaphore;
	}

	// Otherwise, we need to create one, and don't keep track of it, app will release.
	return device.get_handle().createSemaphore(vk::SemaphoreCreateInfo{});
}

void HPPSemaphorePool::release_owned_semaphore(vk::Semaphore semaphore)
{
	// We cannot reuse this semaphore until ::reset().
	released_semaphores.push_back(semaphore);
}

vk::Semaphore HPPSemaphorePool::request_semaphore()
{
	// Check if there is an available semaphore
	if (active_semaphore_count < semaphores.size())
	{
		return semaphores[active_semaphore_count++];
	}

	vk::Semaphore semaphore = device.get_handle().createSemaphore(vk::SemaphoreCreateInfo{});
	semaphores.push_back(semaphore);

	active_semaphore_count++;

	return semaphore;
}

void HPPSemaphorePool::reset()
{
	active_semaphore_count = 0;

	// Now we can safely recycle the released semaphores.
	for (auto &sem : released_semaphores)
	{
		semaphores.push_back(sem);
	}

	released_semaphores.clear();
}

uint32_t HPPSemaphorePool::get_active_semaphore_count() const
{
	return active_semaphore_count;
}
}        // namespace vkb
