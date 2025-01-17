/*
 * Copyright © 2025 Collabora Ltd.
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

#include "window_system_plugin.h"
#include "swapchain_window_system.h"
#include "headless_native_system.h"
#include "window_system_priority.h"

#include "log.h"
#include "options.h"

void vkmark_window_system_load_options(Options&)
{
}

int vkmark_window_system_probe(Options const&)
{
    return VKMARK_WINDOW_SYSTEM_PROBE_OK + VKMARK_HEADLESS_WINDOW_SYSTEM_PRIORITY;
}

std::unique_ptr<WindowSystem> vkmark_window_system_create(Options const& options)
{
    vk::Extent2D size;

    if (options.size.first < 0 || options.size.second < 0)
    {
        Log::warning("HeadlessWindowSystemPlugin: Ignoring invalid size, using 800x600\n");
        size.setWidth(800);
        size.setHeight(600);
    }
    else
    {
        size.setWidth(static_cast<uint32_t>(options.size.first));
        size.setHeight(static_cast<uint32_t>(options.size.second));
    }

    return std::make_unique<SwapchainWindowSystem>(
        std::make_unique<HeadlessNativeSystem>(size),
        options.present_mode,
        options.pixel_format);
}
