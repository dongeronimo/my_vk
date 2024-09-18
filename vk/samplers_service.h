#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <map>
namespace vk {
    const std::string LINEAR_REPEAT_NORMALIZED_NONMIPMAP_SAMPLER = "LinearRepeatNormalizedNonMipmapSampler";
    class SamplerService {
    public:
        SamplerService();
        ~SamplerService();
    private:
        std::map<size_t, VkSampler> mSamplers;
        
        VkSampler LinearRepeatNormalizedNonMipmap();
    };
}