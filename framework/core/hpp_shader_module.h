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

#if defined(VK_USE_PLATFORM_XLIB_KHR)
#	undef None
#endif

namespace vkb
{
namespace core
{
/// Types of shader resources
enum class HPPShaderResourceType
{
	Input,
	InputAttachment,
	Output,
	Image,
	ImageSampler,
	ImageStorage,
	Sampler,
	BufferUniform,
	BufferStorage,
	PushConstant,
	SpecializationConstant,
	All
};

/// This determines the type and method of how descriptor set should be created and bound
enum class HPPShaderResourceMode
{
	Static,
	Dynamic,
	UpdateAfterBind
};

/// A bitmask of qualifiers applied to a resource
struct HPPShaderResourceQualifiers
{
	enum : uint32_t
	{
		None        = 0,
		NonReadable = 1,
		NonWritable = 2,
	};
};


/// Store shader resource data.
/// Used by the shader module.
struct HPPShaderResource
{
	vk::ShaderStageFlags  stages;
	HPPShaderResourceType type;
	HPPShaderResourceMode mode;
	uint32_t              set;
	uint32_t              binding;
	uint32_t              location;
	uint32_t              input_attachment_index;
	uint32_t              vec_size;
	uint32_t              columns;
	uint32_t              array_size;
	uint32_t              offset;
	uint32_t              size;
	uint32_t              constant_id;
	uint32_t              qualifiers;
	std::string           name;
};

/**
 * @brief Adds support for C style preprocessor macros to glsl shaders
 *        enabling you to define or undefine certain symbols
 */
class HPPShaderVariant
{
  public:
	HPPShaderVariant() = default;

	HPPShaderVariant(std::string &&preamble, std::vector<std::string> &&processes);

	size_t get_id() const;

	/**
	 * @brief Add definitions to shader variant
	 * @param definitions Vector of definitions to add to the variant
	 */
	void add_definitions(const std::vector<std::string> &definitions);

	/**
	 * @brief Adds a define macro to the shader
	 * @param def String which should go to the right of a define directive
	 */
	void add_define(const std::string &def);

	/**
	 * @brief Adds an undef macro to the shader
	 * @param undef String which should go to the right of an undef directive
	 */
	void add_undefine(const std::string &undef);

	/**
	 * @brief Specifies the size of a named runtime array for automatic reflection. If already specified, overrides the size.
	 * @param runtime_array_name String under which the runtime array is named in the shader
	 * @param size Integer specifying the wanted size of the runtime array (in number of elements, not size in bytes), used for automatic allocation of buffers.
	 * See get_declared_struct_size_runtime_array() in spirv_cross.h
	 */
	void add_runtime_array_size(const std::string &runtime_array_name, size_t size);

	void set_runtime_array_sizes(const std::unordered_map<std::string, size_t> &sizes);

	const std::string &get_preamble() const;

	const std::vector<std::string> &get_processes() const;

	const std::unordered_map<std::string, size_t> &get_runtime_array_sizes() const;

	void clear();

  private:
	size_t id;

	std::string preamble;

	std::vector<std::string> processes;

	std::unordered_map<std::string, size_t> runtime_array_sizes;

	void update_id();
};

class HPPShaderSource
{
  public:
	HPPShaderSource() = default;

	HPPShaderSource(const std::string &filename);

	size_t get_id() const;

	const std::string &get_filename() const;

	void set_source(const std::string &source);

	const std::string &get_source() const;

  private:
	size_t id;

	std::string filename;

	std::string source;
};


/**
 * @brief Contains shader code, with an entry point, for a specific shader stage.
 * It is needed by a PipelineLayout to create a Pipeline.
 * HPPShaderModule can do auto-pairing between shader code and textures.
 * The low level code can change bindings, just keeping the name of the texture.
 * Variants for each texture are also generated, such as HAS_BASE_COLOR_TEX.
 * It works similarly for attribute locations. A current limitation is that only set 0
 * is considered. Uniform buffers are currently hardcoded as well.
 */
class HPPShaderModule
{
  public:
	HPPShaderModule(HPPDevice               &device,
	             vk::ShaderStageFlagBits stage,
	             const HPPShaderSource   &glsl_source,
	             const std::string    &entry_point,
	             const HPPShaderVariant  &shader_variant);

	HPPShaderModule(const HPPShaderModule &) = delete;

	HPPShaderModule(HPPShaderModule &&other);

	HPPShaderModule &operator=(const HPPShaderModule &) = delete;

	HPPShaderModule &operator=(HPPShaderModule &&) = delete;

	size_t get_id() const;

	vk::ShaderStageFlagBits get_stage() const;

	const std::string &get_entry_point() const;

	const std::vector<HPPShaderResource> &get_resources() const;

	const std::string &get_info_log() const;

	const std::vector<uint32_t> &get_binary() const;

	inline const std::string &get_debug_name() const
	{
		return debug_name;
	}

	inline void set_debug_name(const std::string &name)
	{
		debug_name = name;
	}

	/**
	 * @brief Flags a resource to use a different method of being bound to the shader
	 * @param resource_name The name of the shader resource
	 * @param resource_mode The mode of how the shader resource will be bound
	 */
	void set_resource_mode(const std::string &resource_name, const HPPShaderResourceMode &resource_mode);

  private:
	HPPDevice &device;

	/// Shader unique id
	size_t id;

	/// Stage of the shader (vertex, fragment, etc)
	vk::ShaderStageFlagBits stage{};

	/// Name of the main function
	std::string entry_point;

	/// Human-readable name for the shader
	std::string debug_name;

	/// Compiled source
	std::vector<uint32_t> spirv;

	std::vector<HPPShaderResource> resources;

	std::string info_log;
};

}        // namespace core
}        // namespace vkb
