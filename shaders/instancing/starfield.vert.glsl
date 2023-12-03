#version 450
/* Copyright (c) 2019-2023, Sascha Willems
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

layout (location = 0) out vec3 outUVW;

void main() 
{
	outUVW = vec3((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2, gl_VertexIndex & 2);
	gl_Position = vec4(outUVW.st * 2.0f - 1.0f, 0.0f, 1.0f);
}