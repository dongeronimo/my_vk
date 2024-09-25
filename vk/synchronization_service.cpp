#include "synchronization_service.h"
#include "device.h"
#include "utils\concatenate.h"
#include "vk\debug_utils.h"
namespace vk
{
    SyncronizationService::SyncronizationService()
    {
        mImagesAvailableSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
        mRenderFinishedSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
        mInFlightFence.resize(MAX_FRAMES_IN_FLIGHT);
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;//gotcha: if the fence is unsignalled DrawFrame will hang waiting for it to be signalled during the 1st execution
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (vkCreateSemaphore(vk::Device::gDevice->GetDevice(), 
                &semaphoreInfo, nullptr, &mImagesAvailableSemaphore[i]) != VK_SUCCESS ||
                vkCreateSemaphore(vk::Device::gDevice->GetDevice(), &semaphoreInfo, 
                    nullptr, &mRenderFinishedSemaphore[i]) != VK_SUCCESS ||
                vkCreateFence(vk::Device::gDevice->GetDevice(), &fenceInfo, 
                    nullptr, &mInFlightFence[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create semaphores!");
                auto n0 = Concatenate("mImagesAvailableSemaphore", i);
                auto n1 = Concatenate("mRenderFinishedSemaphore", i);
                auto n2 = Concatenate("mInFlightFence", i);
                SET_NAME(mImagesAvailableSemaphore[i], VK_OBJECT_TYPE_SEMAPHORE, n0.c_str());
                SET_NAME(mRenderFinishedSemaphore[i], VK_OBJECT_TYPE_SEMAPHORE, n1.c_str());
                SET_NAME(mInFlightFence[i], VK_OBJECT_TYPE_FENCE, n2.c_str());
            }
        }
    }
    SyncronizationService::~SyncronizationService()
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
            auto d = vk::Device::gDevice->GetDevice();
            vkDestroySemaphore(d, mImagesAvailableSemaphore[i], nullptr);
            vkDestroySemaphore(d, mRenderFinishedSemaphore[i], nullptr);
            vkDestroyFence(d, mInFlightFence[i], nullptr);
        }
    }
    VkSemaphore& SyncronizationService::ImagesAvailableSemaphore(uint32_t i) 
    {
        return mImagesAvailableSemaphore[i];
    }
    VkSemaphore& SyncronizationService::RenderFinishedSemaphore(uint32_t i) 
    {
        return mRenderFinishedSemaphore[i];
    }
    VkFence& SyncronizationService::InFlightFences(uint32_t i) 
    {
        return mInFlightFence[i];
    }
}