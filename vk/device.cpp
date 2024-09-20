#include "vk/device.h"
#include <cassert>
#include <set>
#include <vector>
#include "vk/debug_utils.h"
#include <stdexcept>
namespace vk {
    Device* Device::gDevice;

    Device::Device(VkPhysicalDevice physicalDevice, VkInstance instance, VkSurfaceKHR surface, std::vector<const char*> validationLayers)
        :mPhysicalDevice(physicalDevice), mInstance(instance), mSurface(surface)
    {
        assert(physicalDevice != VK_NULL_HANDLE);
        assert(instance != VK_NULL_HANDLE);
        assert(surface != VK_NULL_HANDLE);
        assert(gDevice == nullptr);
        gDevice = this;
        //Get the queue families. They can be equal, and in that case that means that
        //the queue can do both presentation and graphics
        auto graphicsQueueFamilyIdx = FindGraphicsQueueFamily(physicalDevice);
        auto presentationQueueFamilyIdx = FindPresentationQueueFamily(physicalDevice,surface);
        this->mGraphicsQueueFamily = *graphicsQueueFamilyIdx;
        this->mPresentationQueueFamily = *presentationQueueFamilyIdx;
        //for each unique queue family id we create a queue info
        std::set<uint32_t> uniqueQueueFamilies = { mGraphicsQueueFamily, mPresentationQueueFamily };
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo  queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            float queuePriority = 1.0f;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
        //description of the logical device
        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        VkPhysicalDeviceFeatures deviceFeatures = {};
        vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
        deviceFeatures.samplerAnisotropy = VK_TRUE;
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
        //The logical device extensions that i want
        const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME, //swapchain
            VK_EXT_DEBUG_MARKER_EXTENSION_NAME //renderdoc marker
        };
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
        //the layers enabled on this logical device
        if (validationLayers.size() > 0) {
            deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            deviceCreateInfo.enabledLayerCount = 0;
        }
        vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &mDevice);
        vkGetDeviceQueue(mDevice, mGraphicsQueueFamily, 0, &mGraphicsQueue);
        vkGetDeviceQueue(mDevice, mPresentationQueueFamily, 0, &mPresentationQueue);
        /*vkCmdDebugMarkerBeginEXT = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(mDevice, "vkCmdDebugMarkerBeginEXT");
        vkCmdDebugMarkerEndEXT = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(mDevice, "vkCmdDebugMarkerEndEXT");
        vkCmdDebugMarkerInsertEXT = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(mDevice, "vkCmdDebugMarkerInsertEXT");
        *///initializes the object namer
        vk::ObjectNamer::Instance().Init(mDevice);
        //creates the command pool
        VkCommandPoolCreateInfo commandPoolCreateInfo{};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolCreateInfo.queueFamilyIndex = mGraphicsQueueFamily;
        if (vkCreateCommandPool(mDevice, 
            &commandPoolCreateInfo, nullptr, &mCommandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
        SET_NAME(mCommandPool, VK_OBJECT_TYPE_COMMAND_POOL, "MainCommandPool");
    }
    Device::~Device()
    {
        vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
        vkDestroyDevice(mDevice, nullptr);
    }
    VkCommandBuffer Device::CreateCommandBuffer(const std::string& name)
    {
        //a temporary command buffer for copy
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = mCommandPool;
        allocInfo.commandBufferCount = 1;
        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(mDevice, &allocInfo, &commandBuffer);
        SET_NAME(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER, name.c_str());
        return commandBuffer;
    }
    void Device::BeginRecordingCommands(VkCommandBuffer cmd)
    {
        //begin recording commands
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; //this command buffer will be used only once
        vkBeginCommandBuffer(cmd, &beginInfo);
    }
    void Device::SubmitAndFinishCommands(VkCommandBuffer cmd)
    {
        //end the recording
        vkEndCommandBuffer(cmd);
        //submit the command
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;

        vkQueueSubmit(mGraphicsQueue,
            1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(mGraphicsQueue);//cpu waits until copy is done
        //free the command buffer
        vkFreeCommandBuffers(mDevice, mCommandPool, 1, &cmd);
    }
    void Device::CopyBuffer(VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size, VkCommandBuffer cmd, VkBuffer src, VkBuffer dst)
    {
        //copy command
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = srcOffset; // Optional
        copyRegion.dstOffset = dstOffset; // Optional
        copyRegion.size = size;
        vkCmdCopyBuffer(cmd, src, dst, 1, &copyRegion);
    }
    std::optional<uint32_t> Device::FindGraphicsQueueFamily(VkPhysicalDevice device)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
        int i = 0;
        std::optional<uint32_t> result;
        for (const auto& queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                result = i;
            }
            i++;
        }
        return result;
    }
    std::optional<uint32_t> Device::FindPresentationQueueFamily(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
        int i = 0;
        std::optional<uint32_t> result;
        for (const auto& queueFamily : queueFamilies)
        {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport) {
                result = i;
                break;
            }
            i++;
        }
        return result;
    }
}