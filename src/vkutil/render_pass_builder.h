/*
 * Copyright © 2017 Collabora Ltd.
 *
 * This file is part of vkmark.
 *
 * vkmark is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * vkmark is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with vkmark. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Alexandros Frantzis <alexandros.frantzis@collabora.com>
 */

#pragma once

#include <vulkan/vulkan.hpp>

#include "managed_resource.h"

class VulkanState;

namespace vkutil
{

class RenderPassBuilder
{
public:
    RenderPassBuilder(VulkanState& vulkan);

    RenderPassBuilder& set_color_format(vk::Format format);
    RenderPassBuilder& set_depth_format(vk::Format format);

    RenderPassBuilder& set_color_load_op(vk::AttachmentLoadOp load_op);

    ManagedResource<vk::RenderPass> build();

private:
    VulkanState& vulkan;
    std::vector<vk::Format> color_formats;
    vk::Format depth_format;
    std::vector<vk::AttachmentLoadOp> color_load_ops;
};

}
