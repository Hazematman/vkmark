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

#include "scene.h"
#include "managed_resource.h"

#include <vulkan/vulkan.hpp>

class ClearScene : public Scene
{
public:
    ClearScene();

    void setup(VulkanState&, std::vector<VulkanImage> const&) override;
    void teardown() override;

    VulkanImage draw(VulkanImage const&) override;
    void update() override;

private:
    void prepare_command_buffer(VulkanImage const& image);
    void setup_rts();
    void setup_renderpass();
    void setup_framebuffers(std::vector<VulkanImage> const& vulkan_images);

    enum {
        CLEAR_CMD,
        CLEAR_LOADOP,
    } mode;

    VulkanState* vulkan;
    vk::Extent2D extent;
    vk::Format format;
    ManagedResource<vk::RenderPass> render_pass;
    std::vector<vk::CommandBuffer> command_buffers;
    ManagedResource<vk::Semaphore> submit_semaphore;
    std::vector<std::vector<ManagedResource<vk::ImageView>>> image_views;
    std::vector<ManagedResource<vk::Framebuffer>> framebuffers;

    std::vector<ManagedResource<vk::Image>> rt_images;
    vk::ClearColorValue clear_color;
    bool cycle;
    int num_rts;
};
