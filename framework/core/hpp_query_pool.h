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
namespace core
{

/**
 * @brief facade class around vkb::QueryPool, providing a vulkan.hpp-based interface
 *
 * See vkb::QueryPool for documentation
 */
class HPPQueryPool
{
  public:
	/**
	 * @brief Creates a Vulkan Query Pool
	 * @param d The device to use
	 * @param info Creation details
	 */
	HPPQueryPool(HPPDevice &d, const vk::QueryPoolCreateInfo &info);
#if 0

	HPPQueryPool(const HPPQueryPool &) = delete;

	HPPQueryPool(HPPQueryPool &&pool) :
	    device{other.device},
	    handle{other.handle}
	{
		other.handle = nullptr;
	}

	~HPPQueryPool()
	{
		if (handle)
		{
			device.get_handle().desotry(handle);
		}
	}

	HPPQueryPool &operator=(const QueryPool &) = delete;

	HPPQueryPool &operator=(QueryPool &&) = delete;

	vk::QueryPool get_handle() const
	{
		assert(handle != VK_NULL_HANDLE && "QueryPool handle is invalid");
		return handle;
	}

	/**
	 * @brief Reset a range of queries in the query pool. Only call if VK_EXT_host_query_reset is enabled.
	 * @param first_query The first query to reset
	 * @param query_count The number of queries to reset
	 */
	void host_reset(uint32_t first_query, uint32_t query_count)
	{
		assert(device.is_enabled("VK_EXT_host_query_reset") &&
		       "VK_EXT_host_query_reset needs to be enabled to call QueryPool::host_reset");

		device.get_handle().resetQueryPoolEXT(get_handle(), first_query, query_count);
	}

	/**
	 * @brief Get query pool results
	 * @param first_query The initial query index
	 * @param num_queries The number of queries to read
	 * @param result_bytes The number of bytes in the results array
	 * @param results Array of bytes result_bytes long
	 * @param stride The stride in bytes between results for individual queries
	 * @param flags A bitmask of VkQueryResultFlagBits
	 */
	VkResult get_results(uint32_t first_query, uint32_t num_queries,
	                     size_t result_bytes, void *results, VkDeviceSize stride,
	                     VkQueryResultFlags flags)
	{
		device.get_handle().getQueryPoolResults(get_handle(), first_query, num_queries,
		                                        result_bytes, results, stride, flags);
	}

  private:
#endif
	HPPDevice &device;

	vk::QueryPool handle;
};

}        // namespace core
}        // namespace vkb
