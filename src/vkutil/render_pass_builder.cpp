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

#include "render_pass_builder.h"

#include "vulkan_state.h"

vkutil::RenderPassBuilder::RenderPassBuilder(VulkanState& vulkan)
    : vulkan{vulkan},
      depth_format{vk::Format::eUndefined}
{
}

vkutil::RenderPassBuilder& vkutil::RenderPassBuilder::set_color_format(vk::Format format_)
{
    color_formats.push_back(format_);
    return *this;
}

vkutil::RenderPassBuilder& vkutil::RenderPassBuilder::set_depth_format(vk::Format format_)
{
    depth_format = format_;
    return *this;
}

vkutil::RenderPassBuilder& vkutil::RenderPassBuilder::set_color_load_op(vk::AttachmentLoadOp load_op_)
{
    color_load_ops.push_back(load_op_);
    return *this;
}

ManagedResource<vk::RenderPass> vkutil::RenderPassBuilder::build()
{
    size_t attachment_index = 0;
    std::vector<vk::AttachmentDescription> attachments;
    std::vector<vk::AttachmentReference> attachment_references;
    for (auto color_format : color_formats)
    {
        attachments.push_back(
            vk::AttachmentDescription{}
                .setFormat(color_format)
                .setSamples(vk::SampleCountFlagBits::e1)
                .setLoadOp(color_load_ops[attachment_index])
                .setStoreOp(vk::AttachmentStoreOp::eStore)
                .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                .setInitialLayout(vk::ImageLayout::eUndefined)
                .setFinalLayout(vk::ImageLayout::ePresentSrcKHR));

        attachment_references.push_back(
            vk::AttachmentReference{}
                .setAttachment(attachment_index++)
                .setLayout(vk::ImageLayout::eColorAttachmentOptimal));
    }

    auto const depth_attachment = vk::AttachmentDescription{}
        .setFormat(depth_format)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    auto const depth_attachment_ref = vk::AttachmentReference{}
        .setAttachment(attachment_index)
        .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    bool const use_depth_attachment = depth_format != vk::Format::eUndefined;

    auto const subpass = vk::SubpassDescription{}
        .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachmentCount(attachment_index)
        .setPColorAttachments(attachment_references.data())
        .setPDepthStencilAttachment(use_depth_attachment ? &depth_attachment_ref : nullptr);

    if (use_depth_attachment)
        attachments.push_back(depth_attachment);

    auto const subpass_dependency = vk::SubpassDependency{}
        .setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setSrcAccessMask({})
        .setDstSubpass(0)
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead |
                          vk::AccessFlagBits::eColorAttachmentWrite)
        .setDependencyFlags(vk::DependencyFlagBits::eByRegion);

    auto const render_pass_create_info = vk::RenderPassCreateInfo{}
        .setAttachmentCount(attachments.size())
        .setPAttachments(attachments.data())
        .setSubpassCount(1)
        .setPSubpasses(&subpass)
        .setDependencyCount(1)
        .setPDependencies(&subpass_dependency);

    return ManagedResource<vk::RenderPass>{
        vulkan.device().createRenderPass(render_pass_create_info),
        [vptr=&vulkan] (auto const& rp) { vptr->device().destroyRenderPass(rp); }};
}
