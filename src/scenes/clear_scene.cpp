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

#include "clear_scene.h"

#include "util.h"
#include "vulkan_state.h"
#include "vulkan_image.h"
#include "vkutil/vkutil.h"

#include <cmath>

ClearScene::ClearScene() : Scene{"clear"}, cycle{true}
{
    options_["color"] = SceneOption("color", "cycle",
                                    "The normalized (0.0-1.0) \"r,g,b,a\" color to use or \"cycle\" to cycle");
    options_["clear-mode"] = SceneOption("clear-mode", "cmd",
                                         "The operation to perform the clear by",
                                         "cmd,loadop");
}

void ClearScene::setup_renderpass()
{
    render_pass = vkutil::RenderPassBuilder(*vulkan)
        .set_color_format(format)
        .set_color_load_op(vk::AttachmentLoadOp::eClear)
        .build();
}

void ClearScene::setup_framebuffers(std::vector<VulkanImage> const& vulkan_images)
{
    for (auto const& vulkan_image : vulkan_images)
    {
        image_views.push_back(
            vkutil::ImageViewBuilder{*vulkan}
                .set_image(vulkan_image.image)
                .set_format(vulkan_image.format)
                .set_aspect_mask(vk::ImageAspectFlagBits::eColor)
                .build());
    }

    for (auto const& image_view : image_views)
    {
        framebuffers.push_back(
            vkutil::FramebufferBuilder{*vulkan}
                .set_render_pass(render_pass)
                .set_image_views({image_view})
                .set_extent(extent)
                .build());
    }
}

void ClearScene::setup(VulkanState& vulkan_, std::vector<VulkanImage> const& images)
{
    Scene::setup(vulkan_, images);

    vulkan = &vulkan_;
    extent = images[0].extent;
    format = images[0].format;

    auto const command_buffer_allocate_info = vk::CommandBufferAllocateInfo{}
        .setCommandPool(vulkan->command_pool())
        .setCommandBufferCount(images.size())
        .setLevel(vk::CommandBufferLevel::ePrimary);

    command_buffers = vulkan->device().allocateCommandBuffers(command_buffer_allocate_info);
    submit_semaphore = vkutil::SemaphoreBuilder{*vulkan}.build();

    if (options_["color"].value == "cycle")
    {
        cycle = true;
        std::array<float,4> const color_value{{1.0f,0.0f,0.0f,1.0f}};
        clear_color = vk::ClearColorValue{color_value};
    }
    else
    {
        cycle = false;
        auto const components = Util::split(options_["color"].value, ',');
        std::array<float,4> color_value{{0.0f,0.0f,0.0f,1.0f}};

        if (components.size() > color_value.size())
            throw std::runtime_error("too many components in \"color\" option");

        for (size_t i = 0; i < components.size(); ++i)
            color_value[i] = std::stof(components[i]);

        clear_color = vk::ClearColorValue{color_value};
    }

    auto const& clear_op = options_["clear-mode"].value;

    if (clear_op == "cmd")
        mode = CLEAR_CMD;
    else if (clear_op == "loadop")
        mode = CLEAR_LOADOP;
    else
        throw std::runtime_error("Invalid \"clear-mode\" option");

    if (mode == CLEAR_LOADOP) {
        setup_framebuffers(images);
        setup_renderpass();
    }
}

void ClearScene::teardown()
{
    vulkan->device().waitIdle();

    submit_semaphore = {};

    vulkan->device().freeCommandBuffers(vulkan->command_pool(), command_buffers);
    framebuffers.clear();
    image_views.clear();
    render_pass = {};

    Scene::teardown();
}

void ClearScene::prepare_command_buffer(VulkanImage const& image)
{
    auto const begin_info = vk::CommandBufferBeginInfo{}
        .setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

    auto const i = image.index;

    command_buffers[i].begin(begin_info);

    if (mode == CLEAR_CMD) {
        auto const image_range = vk::ImageSubresourceRange{}
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseMipLevel(0)
            .setLevelCount(1)
            .setBaseArrayLayer(0)
            .setLayerCount(1);

        auto const undef_to_transfer_barrier = vk::ImageMemoryBarrier{}
            .setImage(image.image)
            .setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
            .setSrcAccessMask({})
            .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setSubresourceRange(image_range);

        auto const transfer_to_present_barrier = vk::ImageMemoryBarrier{}
            .setImage(image.image)
            .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
            .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask({})
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setSubresourceRange(image_range);

        command_buffers[i].pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eTransfer,
            {}, {}, {},
            undef_to_transfer_barrier);

        command_buffers[i].clearColorImage(
            image.image,
            vk::ImageLayout::eTransferDstOptimal,
            clear_color,
            image_range);

        command_buffers[i].pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eBottomOfPipe,
            {}, {}, {},
            transfer_to_present_barrier);
    } else if(mode == CLEAR_LOADOP) {
        std::array<vk::ClearValue, 1> clear_values{clear_color};
        auto const render_pass_begin_info = vk::RenderPassBeginInfo{}
            .setRenderPass(render_pass)
            .setFramebuffer(framebuffers[i])
            .setRenderArea({{0,0}, extent})
            .setClearValueCount(clear_values.size())
            .setPClearValues(clear_values.data());

        command_buffers[i].beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
        command_buffers[i].endRenderPass();
    }

    command_buffers[i].end();
}

VulkanImage ClearScene::draw(VulkanImage const& image)
{
    prepare_command_buffer(image);

    vk::PipelineStageFlags mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    auto const submit_info = vk::SubmitInfo{}
        .setSignalSemaphoreCount(1)
        .setPSignalSemaphores(&submit_semaphore.raw)
        .setCommandBufferCount(1)
        .setPCommandBuffers(&command_buffers[image.index])
        .setWaitSemaphoreCount(image.semaphore ? 1 : 0)
        .setPWaitSemaphores(&image.semaphore)
        .setPWaitDstStageMask(&mask);

    vulkan->graphics_queue().submit(submit_info, image.fence);

    return image.copy_with_semaphore(submit_semaphore);
}

void ClearScene::update()
{
    auto const elapsed = Util::get_timestamp_us() - start_time;

    if (cycle)
    {
        // HSV to RGB conversion for S=V=1 and H completeling a cycle every 5sec
        double const period = 5000000.0;
        float const c = 1.0;
        float const h = (360 / 60) * std::fmod(elapsed, period) / period;
        float const x = c * (1 - std::fabs(std::fmod(h, 2.0) - 1));
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;

        switch (static_cast<int>(h))
        {
            case 0: r = c; g = x; b = 0; break;
            case 1: r = x; g = c; b = 0; break;
            case 2: r = 0; g = c; b = x; break;
            case 3: r = 0; g = x; b = c; break;
            case 4: r = x; g = 0; b = c; break;
            case 5: r = c; g = 0; b = x; break;
            default: r = g = b = 0; break;
        };

        clear_color = vk::ClearColorValue{std::array<float,4>{{r, g, b, 1.0f}}};
    }

    Scene::update();
}
