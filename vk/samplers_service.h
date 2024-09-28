#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <map>
#include "utils/hash.h"
namespace vk {
    const std::string LINEAR_REPEAT_NORMALIZED_NONMIPMAP_SAMPLER = "LinearRepeatNormalizedNonMipmapSampler";
    class SamplerService {
    public:
        SamplerService();
        ~SamplerService();
        const VkSampler GetSampler(const std::string& name) {
            return mSamplers.at(utils::Hash(name));
        }
    private:
        std::map<hash_t, VkSampler> mSamplers;
        
        VkSampler LinearRepeatNormalizedNonMipmap();
    };
}