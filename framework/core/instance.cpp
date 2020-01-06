/* Copyright (c) 2018-2020, Arm Limited and Contributors
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

#include "instance.h"

#include <algorithm>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace vkb
{
namespace
{
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT rawflags, VkDebugReportObjectTypeEXT /*type*/,
                                                       uint64_t /*object*/, size_t /*location*/, int32_t /*message_code*/,
                                                       const char *layer_prefix, const char *message, void * /*user_data*/)
{
	vk::DebugReportFlagsEXT flags{rawflags};
	if (flags & vk::DebugReportFlagBitsEXT::eError)
	{
		LOGE("{}: {}", layer_prefix, message);
	}
	else if (flags & vk::DebugReportFlagBitsEXT::eWarning)
	{
		LOGW("{}: {}", layer_prefix, message);
	}
	else if (flags & vk::DebugReportFlagBitsEXT::ePerformanceWarning)
	{
		LOGW("{}: {}", layer_prefix, message);
	}
	else
	{
		LOGI("{}: {}", layer_prefix, message);
	}
	return VK_FALSE;
}
#endif

bool validate_extensions(const std::vector<const char *> &           required,
                         const std::vector<vk::ExtensionProperties> &available)
{
	for (auto extension : required)
	{
		bool found = false;
		for (auto &available_extension : available)
		{
			if (strcmp(available_extension.extensionName, extension) == 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			LOGE("Extension {} not found", extension);
			return false;
		}
	}

	return true;
}

bool validate_layers(const std::vector<const char *> &       required,
                     const std::vector<vk::LayerProperties> &available)
{
	for (auto layer : required)
	{
		bool found = false;
		for (auto &available_layer : available)
		{
			if (strcmp(available_layer.layerName, layer) == 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			LOGE("Validation Layer {} not found", layer);
			return false;
		}
	}

	return true;
}
}        // namespace

std::vector<const char *> get_optimal_validation_layers(const std::vector<vk::LayerProperties> &supported_instance_layers)
{
	std::vector<std::vector<const char *>> validation_layer_priority_list =
	    {
	        // The preferred validation layer is "VK_LAYER_KHRONOS_validation"
	        {"VK_LAYER_KHRONOS_validation"},

	        // Otherwise we fallback to using the LunarG meta layer
	        {"VK_LAYER_LUNARG_standard_validation"},

	        // Otherwise we attempt to enable the individual layers that compose the LunarG meta layer since it doesn't exist
	        {
	            "VK_LAYER_GOOGLE_threading",
	            "VK_LAYER_LUNARG_parameter_validation",
	            "VK_LAYER_LUNARG_object_tracker",
	            "VK_LAYER_LUNARG_core_validation",
	            "VK_LAYER_GOOGLE_unique_objects",
	        },

	        // Otherwise as a last resort we fallback to attempting to enable the LunarG core layer
	        {"VK_LAYER_LUNARG_core_validation"}};

	for (auto &validation_layers : validation_layer_priority_list)
	{
		if (validate_layers(validation_layers, supported_instance_layers))
		{
			return validation_layers;
		}

		LOGW("Couldn't enable validation layers (see log for error) - falling back");
	}

	// Else return nothing
	return {};
}

Instance::Instance(const std::string &              application_name,
                   const std::vector<const char *> &required_extensions,
                   const std::vector<const char *> &required_validation_layers,
                   bool                             headless) :
    extensions{required_extensions}
{
	VkResult result = volkInitialize();
	if (result)
	{
		vk::throwResultException(static_cast<vk::Result>(result), "Failed to initialize volk.");
	}
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

	auto available_instance_extensions = vk::enumerateInstanceExtensionProperties();

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

	// Try to enable headless surface extension if it exists
	if (headless)
	{
		bool headless_extension = false;
		for (auto &available_extension : available_instance_extensions)
		{
			if (strcmp(available_extension.extensionName, VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME) == 0)
			{
				headless_extension = true;
				LOGI("{} is available, enabling it", VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
				extensions.push_back(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
			}
		}
		if (!headless_extension)
		{
			LOGW("{} is not available, disabling swapchain creation", VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
		}
	}
	else
	{
		extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	}

	if (!validate_extensions(extensions, available_instance_extensions))
	{
		throw std::runtime_error("Required instance extensions are missing.");
	}

	auto supported_validation_layers = vk::enumerateInstanceLayerProperties();

	std::vector<const char *> requested_validation_layers(required_validation_layers);

#ifdef VKB_VALIDATION_LAYERS
	// Determine the optimal validation layers to enable that are necessary for useful debugging
	std::vector<const char *> optimal_validation_layers = get_optimal_validation_layers(supported_validation_layers);
	requested_validation_layers.insert(requested_validation_layers.end(), optimal_validation_layers.begin(), optimal_validation_layers.end());
#endif

	if (validate_layers(requested_validation_layers, supported_validation_layers))
	{
		LOGI("Enabled Validation Layers:")
		for (const auto &layer : requested_validation_layers)
		{
			LOGI("	\t{}", layer);
		}
	}
	else
	{
		throw std::runtime_error("Required validation layers are missing.");
	}

	vk::ApplicationInfo app_info;
	app_info.pApplicationName   = application_name.c_str();
	app_info.applicationVersion = 0;
	app_info.pEngineName        = "Vulkan Samples";
	app_info.engineVersion      = 0;
	app_info.apiVersion         = VK_MAKE_VERSION(1, 0, 0);

	vk::InstanceCreateInfo instance_info;

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	vk::DebugReportCallbackCreateInfoEXT debug_report_info;
	debug_report_info.flags          = vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning;
	debug_report_info.pfnCallback    = (PFN_vkDebugReportCallbackEXT) debug_callback;
	instance_info.pNext = &debug_report_info;
#endif

	instance_info.pApplicationInfo = &app_info;

	instance_info.enabledExtensionCount   = to_u32(extensions.size());
	instance_info.ppEnabledExtensionNames = extensions.data();

	instance_info.enabledLayerCount   = to_u32(requested_validation_layers.size());
	instance_info.ppEnabledLayerNames = requested_validation_layers.data();

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	VkDebugReportCallbackCreateInfoEXT debug_report_create_info = {VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT};
	debug_report_create_info.flags                              = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	debug_report_create_info.pfnCallback                        = debug_callback;

	instance_info.pNext = &debug_report_create_info;
#endif

	// Create the Vulkan instance
	static_cast<vk::Instance &>(*this) = vk::createInstance(instance_info);

	volkLoadInstance(get_handle());
	VULKAN_HPP_DEFAULT_DISPATCHER.init(*this);

	query_gpus();
}

Instance::Instance(vk::Instance instance)
{
	auto &self = static_cast<vk::Instance &>(*this);
	self       = instance;
	if (self)
	{
		query_gpus();
	}
	else
	{
		throw std::runtime_error("Instance not valid");
	}
}

Instance::~Instance()
{
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	if (debug_report_callback.operator bool())
	{
		destroyDebugReportCallbackEXT(debug_report_callback);
	}
#endif

	auto &self = static_cast<vk::Instance &>(*this);
	if (self)
	{
		destroy();
	}
}

void Instance::query_gpus()
{
	// Querying valid physical devices on the machine
	gpus = enumeratePhysicalDevices();
}

vk::PhysicalDevice Instance::get_gpu()
{
	// Find a discrete GPU
	for (auto gpu : gpus)
	{
		vk::PhysicalDeviceProperties properties = gpu.getProperties();
		if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
		{
			return gpu;
		}
	}

	// Otherwise just pick the first one
	LOGW("Couldn't find a discrete physical device, using integrated graphics");
	return gpus.at(0);
}

bool Instance::is_enabled(const char *extension)
{
	return std::find(extensions.begin(), extensions.end(), extension) != extensions.end();
}

vk::Instance Instance::get_handle() const
{
	return static_cast<const vk::Instance &>(*this);
}

const std::vector<const char *> &Instance::get_extensions()
{
	return extensions;
}
}        // namespace vkb
