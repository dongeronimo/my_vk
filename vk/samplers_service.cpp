#include "samplers_service.h"
#include "instance.h"
#include "device.h"
#include "debug_utils.h"
#include <utils/hash.h>
namespace vk {
    SamplerService::SamplerService()
    {
        mSamplers.insert({
            utils::Hash(LINEAR_REPEAT_NORMALIZED_NONMIPMAP_SAMPLER),
            LinearRepeatNormalizedNonMipmap()
            });
    }

    SamplerService::~SamplerService()
    {
        for (auto& kv : mSamplers) {
            vkDestroySampler(vk::Device::gDevice->GetDevice(),
                kv.second, nullptr);
        }
        mSamplers.clear();
    }

    VkSampler SamplerService::LinearRepeatNormalizedNonMipmap()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(vk::Instance::gInstance->GetPhysicalDevice(), 
            &properties);
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;
        VkSampler sampler;
        if (vkCreateSampler(vk::Device::gDevice->GetDevice(), &samplerInfo, 
            nullptr, &sampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
        SET_NAME(sampler, VK_OBJECT_TYPE_SAMPLER, 
            LINEAR_REPEAT_NORMALIZED_NONMIPMAP_SAMPLER.c_str());
        return sampler;
    }


}
