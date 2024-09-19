#pragma once
#include <vulkan/vulkan.h>
#include <string>
namespace entities {
    struct Image {
        VkImage mImage;
        VkImageView mImageView;
        const std::string mName;
    };
}