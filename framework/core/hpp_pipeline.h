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

#include <common/hpp_vk_common.h>
#include <rendering/hpp_pipeline_state.h>

namespace vkb
{
namespace rendering
{
class HPPPipelineState;
}

namespace core
{
class HPPDevice;

class HPPPipeline
{
  public:
	using PipelineState = rendering::HPPPipelineState;
	HPPPipeline(HPPDevice &device);

	HPPPipeline(const HPPPipeline &) = delete;

	HPPPipeline(HPPPipeline &&other);

	virtual ~HPPPipeline();

	HPPPipeline &operator=(const HPPPipeline &) = delete;

	HPPPipeline &operator=(HPPPipeline &&) = delete;

	vk::Pipeline get_handle() const;

	const PipelineState &get_state() const;

  protected:
	HPPDevice &device;

	vk::Pipeline handle;

	rendering::HPPPipelineState state;
};

class HPPComputePipeline : public HPPPipeline
{
  public:
	HPPComputePipeline(HPPComputePipeline &&) = default;

	virtual ~HPPComputePipeline() = default;

	HPPComputePipeline(HPPDevice        &device,
	                   vk::PipelineCache pipeline_cache,
	                   PipelineState    &pipeline_state);
};

class HPPGraphicsPipeline : public HPPPipeline
{
  public:
	HPPGraphicsPipeline(HPPGraphicsPipeline &&) = default;

	virtual ~HPPGraphicsPipeline() = default;

	HPPGraphicsPipeline(HPPDevice           &device,
	                    vk::PipelineCache pipeline_cache,
	                    PipelineState    &pipeline_state);
};
}        // namespace core
}        // namespace vkb
