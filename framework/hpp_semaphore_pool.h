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

#include "common/hpp_vk_common.h"

namespace vkb
{
class HPPDevice;

class HPPSemaphorePool
{
  public:
	HPPSemaphorePool(HPPDevice &device);

	HPPSemaphorePool(const HPPSemaphorePool &) = delete;

	HPPSemaphorePool(HPPSemaphorePool &&other) = delete;

	~HPPSemaphorePool();

	HPPSemaphorePool &operator=(const HPPSemaphorePool &) = delete;

	HPPSemaphorePool &operator=(HPPSemaphorePool &&) = delete;

	vk::Semaphore request_semaphore();
	vk::Semaphore request_semaphore_with_ownership();
	void          release_owned_semaphore(vk::Semaphore semaphore);

	void reset();

	uint32_t get_active_semaphore_count() const;

  private:
	HPPDevice &device;

	std::vector<vk::Semaphore> semaphores;
	std::vector<vk::Semaphore> released_semaphores;

	uint32_t active_semaphore_count{0};
};
}        // namespace vkb
