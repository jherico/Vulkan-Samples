/* Copyright (c) 2024, Bradley Austin Davis
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

#include <filesystem/filesystem.hpp>

namespace vkb
{
namespace filesystem
{

class MemoryMappedFile
{
	struct PlatformData;
	using PlatformDataPtr = std::unique_ptr<PlatformData>;

	static void write(const Path &path, const uint8_t *data, size_t size);

	// No copy or move semantics, no subclassing
	MemoryMappedFile()                          = delete;
	MemoryMappedFile(const MemoryMappedFile &)  = delete;
	MemoryMappedFile(const MemoryMappedFile &&) = delete;

  public:
	MemoryMappedFile(const Path &path);
	~MemoryMappedFile();

	const uint8_t *data() const;
	size_t         size() const;

private:
	size_t          bytes{0};
	const uint8_t  *mapped{nullptr};
	PlatformDataPtr platform_data_ptr;
};
}        // namespace filesystem

}        // namespace vkb
