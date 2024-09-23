#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <string>
//The gpu objects namimg api wants the object as uint64. All vk objects are opaque handlers so this macro here is ok
//as long as you are in 64 bits.
#define TO_HANDLE(o) reinterpret_cast<uint64_t>(o)
namespace vk {
    std::vector<const char*> GetValidationLayerNames();

    bool EnableValidationLayers();

    bool CheckValidationLayerSupport();

    void SetValidationLayerSupportAtInstanceCreateInfo(VkInstanceCreateInfo& info);

    void SetupDebugMessenger(VkInstance instance,
        VkDebugUtilsMessengerEXT& debugMessenger);

    void DestroyDebugMessenger(VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger);

    /// <summary>
   /// Utility singleton to name vk entities like buffers. To use it you need to 
   /// first call ObjectNamer::GetInstance().Init and pass a valid VkDevice.
   /// </summary>
    class ObjectNamer {
    public:
        ObjectNamer(const ObjectNamer&) = delete;
        ObjectNamer& operator=(const ObjectNamer&) = delete;
        /// <summary>
        /// You have to set the device once before any calls to SetName or else SetName will throw an assert
        /// </summary>
        /// <param name="device"></param>
        void Init(VkDevice device);
        static ObjectNamer& Instance();
        /// <summary>
        /// Remember to call Init to set the device.
        /// </summary>
        /// <param name="object"></param>
        /// <param name="objectType"></param>
        /// <param name="name"></param>
        void SetName(uint64_t object, VkObjectType objectType, const char* name);
    private:
        static VkDevice gDevice;
        // Private constructor to prevent instantiation
        ObjectNamer();
        ~ObjectNamer() = default;
    };

    void SetMark(std::array<float, 4> color,
        std::string name,
        VkCommandBuffer cmd);

    void EndMark(VkCommandBuffer cmd);
}
#define SET_NAME(obj, type, name) vk::ObjectNamer::Instance().SetName(TO_HANDLE(obj), type, name);