/* Copyright (c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
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

class HPPFencePool
{
  public:
	HPPFencePool(core::HPPDevice &device);

	HPPFencePool(const HPPFencePool &) = delete;

	HPPFencePool(HPPFencePool &&other) = delete;

	~HPPFencePool();

	HPPFencePool &operator=(const HPPFencePool &) = delete;

	HPPFencePool &operator=(HPPFencePool &&) = delete;

	vk::Fence request_fence();

	[[nodiscard]] vk::Result wait(uint32_t timeout = std::numeric_limits<uint32_t>::max()) const;

	[[nodiscard]] vk::Result reset();

  private:
	core::HPPDevice &device;

	std::vector<vk::Fence> fences;

	uint32_t active_fence_count{0};
};
}        // namespace vkb
