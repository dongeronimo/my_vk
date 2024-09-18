#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
namespace vk {
    class ImageService {
    public:
        ImageService(const std::vector<std::string>& assetNames);
    };
}