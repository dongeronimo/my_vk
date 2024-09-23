#include "vk/debug_utils.h"
#include <stdexcept>
#include <cassert>
#include "device.h"
VkDevice vk::ObjectNamer::gDevice;
namespace vk {
#ifdef NDEBUG
    const bool gEnableValidationLayers = false;
#else
    const bool gEnableValidationLayers = true;
#endif
    //Table of validation layers that i'll use
    const std::vector<const char*> gValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    /// <summary>
    /// Debug callback
    /// </summary>
    /// <param name="messageSeverity"></param>
    /// <param name="messageType"></param>
    /// <param name="pCallbackData"></param>
    /// <param name="pUserData"></param>
    /// <returns></returns>
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        if (messageSeverity >= VK_DEBUG_LEVEL)
        {
            printf("Validation Layer:\n "
                "  %s:%d  %s\n",
                //"  msg:%s name:%s\n ", 
                pCallbackData->pMessageIdName,
                pCallbackData->messageIdNumber,
                pCallbackData->pMessage
            );
        }
        return VK_FALSE;
    }


    std::vector<const char*> GetValidationLayerNames()
    {
        return gValidationLayers;
    }

    bool EnableValidationLayers()
    {
        return gEnableValidationLayers;
    }

    bool CheckValidationLayerSupport()
    {
        //how many layers?
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        //get them
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        //all validation layers that i want are present (they are defined at gValidationLayers)
        for (const char* layerName : gValidationLayers)
        {
            bool layerFound = false;
            for (const auto& layerProps : availableLayers)
            {
                if (strcmp(layerName, layerProps.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }
            if (!layerFound)
            {
                return false;
            }
        }
        return true;
    }

    void SetValidationLayerSupportAtInstanceCreateInfo(VkInstanceCreateInfo& info)
    {
        if (gEnableValidationLayers)
        {
            info.enabledLayerCount = static_cast<uint32_t>(gValidationLayers.size());
            info.ppEnabledLayerNames = gValidationLayers.data();
        }
        else {
            info.enabledLayerCount = 0;
        }
    }

    void SetupDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT& debugMessenger)
    {
        //the debug utils description
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr; // Optional
        //vkCreateDebugUtilsMessengerEXT is an extension, i have to get it's pointer manually
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
            "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            VkResult result = func(instance, &createInfo, nullptr, &debugMessenger);
            if (result != VK_SUCCESS)
                throw std::runtime_error("Could not create debug-utils messenger");
        }
        else
        {
            throw std::runtime_error("Failed to load the pointer to vkCreateDebugUtilsMessengerEXT");
        }
    }

    void DestroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, nullptr);
        }
    }
    PFN_vkCmdDebugMarkerBeginEXT __vkCmdDebugMarkerBeginEXT;
    PFN_vkCmdDebugMarkerEndEXT __vkCmdDebugMarkerEndEXT;
    PFN_vkCmdDebugMarkerInsertEXT __vkCmdDebugMarkerInsertEXT;
    void SetMark(std::array<float, 4> color, std::string name, VkCommandBuffer cmd)
    {
        if (__vkCmdDebugMarkerBeginEXT == nullptr) {
            __vkCmdDebugMarkerBeginEXT = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(
                vk::Device::gDevice->GetDevice(), "vkCmdDebugMarkerBeginEXT");
            __vkCmdDebugMarkerEndEXT = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(vk::Device::gDevice->GetDevice(), "vkCmdDebugMarkerEndEXT");

        }
        VkDebugMarkerMarkerInfoEXT markerInfo = {};
        markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
        markerInfo.pNext = nullptr;
        markerInfo.pMarkerName = name.c_str();
        markerInfo.color[0] = color[0];              // Optional RGBA color
        markerInfo.color[1] = color[1];
        markerInfo.color[2] = color[2];
        markerInfo.color[3] = color[3];
        __vkCmdDebugMarkerBeginEXT(cmd, &markerInfo);
    }

    void EndMark(VkCommandBuffer cmd)
    {
        if (__vkCmdDebugMarkerEndEXT == VK_NULL_HANDLE) {
            __vkCmdDebugMarkerEndEXT = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(vk::Device::gDevice->GetDevice(), "vkCmdDebugMarkerEndEXT");
        }
    }

    void vk::ObjectNamer::Init(VkDevice device)
    {
        assert(device != nullptr);
        gDevice = device;
    }

    ObjectNamer& ObjectNamer::Instance()
    {
        static ObjectNamer instance; // Guaranteed to be destroyed, instantiated on first use
        return instance;
    }

    void ObjectNamer::SetName(uint64_t object, VkObjectType objectType, const char* name)
    {
        assert(gDevice != nullptr);
        VkDebugUtilsObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.pNext = nullptr;
        nameInfo.objectType = objectType;
        nameInfo.objectHandle = object;
        nameInfo.pObjectName = name;

        // Get the function pointer for vkSetDebugUtilsObjectNameEXT
        static PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT =
            (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(gDevice,
                "vkSetDebugUtilsObjectNameEXT");

        if (vkSetDebugUtilsObjectNameEXT != nullptr) {
            vkSetDebugUtilsObjectNameEXT(gDevice, &nameInfo);
        }

    }

    vk::ObjectNamer::ObjectNamer()
    {
        gDevice = nullptr;
    }
}