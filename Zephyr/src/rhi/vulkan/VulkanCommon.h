#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#define MAX_FRAME_IN_FLIGHT 2
#include <assert.h>
#include <iostream>

#define VK_CHECK(result, msg) \
    if (result != VK_SUCCESS) \
    { \
        printf("Vulkan Error: %s\n", msg); \
        assert(false); \
    }