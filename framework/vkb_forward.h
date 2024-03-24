/* Copyright (c) 2024, Bradley Austin Davis. All rights reserved.
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

namespace vkb
{
class HPPBufferAllocation;
class HPPBufferBlock;
class HPPBufferPool;
class HPPFencePool;
class HPPResourceBindingState;
class HPPResourceRecord;
class HPPResourceReplay;
class HPPSemaphorePool;

namespace common
{
struct HPPLoadStoreInfo;
}        // namespace common

namespace core
{
class HPPBuffer;
class HPPCommandBuffer;
class HPPCommandPool;
class HPPComputePipeline;
class HPPDescriptorPool;
class HPPDescriptorSet;
class HPPDescriptorSetLayout;
class HPPDevice;
class HPPFramebuffer;
class HPPImageView;
class HPPImage;
class HPPPipelineLayout;
class HPPGraphicsPipeline;
class HPPQueryPool;
class HPPQueue;
class HPPRenderPass;
class HPPSampler;
class HPPShaderModule;
struct HPPShaderResource;
class HPPShaderSource;
class HPPShaderVariant;
struct HPPSubpassInfo;
class HPPSwapchain;

enum class HPPCommandBufferResetMode
{
	ResetPool,
	ResetIndividually,
	AlwaysAllocate,
};

}        // namespace core

namespace rendering
{
struct HPPAttachment;
class HPPLightingState;
struct HPPColorBlendState;
struct HPPDepthStencilState;
struct HPPInputAssemblyState;
struct HPPMultisampleState;
class HPPPipelineState;
struct HPPRasterizationState;
class HPPRenderTarget;
class HPPRenderFrame;
class HPPSubpass;
struct HPPVertexInputState;
struct HPPViewportState;
using HPPRenderTargetCreateFunc = std::function<std::unique_ptr<HPPRenderTarget>(core::HPPImage &&)>;
}        // namespace rendering

}        // namespace vkb