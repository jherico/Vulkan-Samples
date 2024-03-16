/* Copyright (c) 2022-2024, NVIDIA CORPORATION. All rights reserved.
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
namespace common
{
/**
 * @brief facade class around vkb::VulkanException, providing a vulkan.hpp-based interface
 *
 * See vkb::VulkanException for documentation
 */
class HPPVulkanException : public std::runtime_error
{
  public:
	HPPVulkanException(vk::Result result, std::string const &msg = "Vulkan error") :
	    result{result},
	    std::runtime_error{msg},
	    error_message{std::string{std::runtime_error::what()} + " : " + vk::to_string(result)}
	{}

	/**
	 * @brief Returns the Vulkan error code as string
	 * @return String message of exception
	 */
	const char *what() const noexcept override
	{
		return error_message.c_str();
	};

	const vk::Result result;

  private:
	const std::string error_message;
};
}        // namespace common
}        // namespace vkb
