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

#include "ktx_common.h"
#include <filesystem/filesystem.hpp>
#include <stdexcept>

namespace vkb
{
namespace ktx
{
ktxTexture *load_texture(std::string const &filename)
{
	ktxTexture *ktx_texture;

	auto  filesystem_ptr = vkb::filesystem::get();
	auto &filesystem     = *filesystem_ptr;

	KTX_error_code result;
	filesystem.with_file_contents(filename, [&](const uint8_t *data, size_t size) {
		result = ktxTexture_CreateFromMemory(data, size, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx_texture);
	});

	if ((result != KTX_SUCCESS) || (ktx_texture == nullptr))
	{
		throw std::runtime_error("Couldn't load texture");
	}
	return ktx_texture;
}
}        // namespace ktx
}        // namespace vkb