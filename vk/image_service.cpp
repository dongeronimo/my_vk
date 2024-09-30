#include "image_service.h"
#include "io/image-load.h"
#include <algorithm>
#include "device.h"
#include "instance.h"
#include <utils/concatenate.h>
#include "debug_utils.h"
#include <utils/vk_utils.h>
#include "entities/image.h"
#include <utils/hash.h>

void SetImageObjName(VkImage img, const std::string& baseName) {
    auto name = Concatenate(baseName, "ImageObject");
    SET_NAME(img, VK_OBJECT_TYPE_IMAGE, name.c_str());
}

namespace vk
{
    VkImageView ImageService::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
    {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(Device::gDevice->GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image view!");
        }

        return imageView;
    }
    void ImageService::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
    {
        auto device = Device::gDevice->GetDevice();
        auto physicalDevice = Instance::gInstance->GetPhysicalDevice();
        // 1. Image creation
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0; // Optional

        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image!");
        }

        // 2. Memory requirements
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        // 3. Allocate memory for the image
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = utils::FindMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate image memory!");
        }

        // 4. Bind memory to image
        vkBindImageMemory(device, image, imageMemory, 0);
    }
    VkImage ImageService::CreateImage(VkDevice device, uint32_t w, uint32_t h, VkFormat format, VkImageUsageFlags usage)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = w;
        imageInfo.extent.height = h;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;//VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;//VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VkImage image;
        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }
        return image;
    }
    uint32_t ImageService::FindMemoryTypesCompatibleWithAllImages(const std::vector<VkMemoryRequirements>& memoryRequirements)
    {
        uint32_t compatibleMemoryTypes = ~0; // Start with all bits set to 1
        for (const auto& memreq : memoryRequirements) {
            // Intersect with current memoryTypeBits
            compatibleMemoryTypes &= memreq.memoryTypeBits;
        }
        return compatibleMemoryTypes;
    }
    void ImageService::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDevice device)
    {
        assert(size != 0);//size must not be zero
        //Buffer description
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        //create the buffer
        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }
        //the memory the buffer will require, not necessarely equals to the size of the data being stored
        //in it.
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = utils::FindMemoryType(memRequirements.memoryTypeBits, 
            properties);
        //Allocate the memory
        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }
    void ImageService::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandPool commandPool, VkDevice device, VkQueue graphicsQueue)
    {
        auto myDevice = vk::Device::gDevice;
        VkCommandBuffer commandBuffer = myDevice->CreateCommandBuffer("image transition command");//beginSingleTimeCommands(commandPool, device);
        myDevice->BeginRecordingCommands(commandBuffer);
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
        myDevice->SubmitAndFinishCommands(commandBuffer);
    }
    void ImageService::CopyBufferToImage(VkBuffer buffer, VkImage image,
        uint32_t width, uint32_t height)
    {
        auto myDevice = vk::Device::gDevice;
        VkCommandBuffer commandBuffer = myDevice->CreateCommandBuffer("copy buffer to image");
        myDevice->BeginRecordingCommands(commandBuffer);
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = {
            width,
            height,
            1
        };

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );
        myDevice->SubmitAndFinishCommands(commandBuffer);
    }
    ImageService::ImageService(const std::vector<std::string>& assetNames)
    {
        //load the images from the file to the memory
        std::vector<io::ImageData*> imageDataFromFiles(assetNames.size());
        std::transform(assetNames.begin(), assetNames.end(), imageDataFromFiles.begin(),
            [](const std::string& name) {
                io::ImageData* img = io::LoadImage(name);
                img->name = name;
                return img;
            });
        //grab vars that'll need to push the images from the memory to the gpu
        VkDevice device = vk::Device::gDevice->GetDevice();
        VkPhysicalDevice physicalDevice = vk::Instance::gInstance->GetPhysicalDevice();
        VkCommandPool commandPool = vk::Device::gDevice->GetCommandPool();
        VkQueue graphicsQueue = vk::Device::gDevice->GetGraphicsQueue();
        //Calculate the memory requirements
        std::vector<VkImage> vkImages;
        std::vector<VkMemoryRequirements> memoryRequirements;
        VkDeviceSize totalSize = 0;
        VkDeviceSize currentOffset = 0;
        for (int i = 0; i < imageDataFromFiles.size(); i++) {
            VkImage image = CreateImage(device, imageDataFromFiles[i]->w, 
                imageDataFromFiles[i]->h,
                VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
            SetImageObjName(image, imageDataFromFiles[i]->name);
            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(device, image, &memRequirements);
            vkImages.push_back(image);
            memoryRequirements.push_back(memRequirements);
            // Align the current offset to the required alignment of this image
            currentOffset = (currentOffset + memRequirements.alignment - 1) & ~(memRequirements.alignment - 1);
            // Update the total size needed
            totalSize = currentOffset + memRequirements.size;
            // Move the offset forward
            currentOffset += memRequirements.size;
        }
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        //find the memory type that is compatible with all images        
        uint32_t compatibleMemoryTypes = FindMemoryTypesCompatibleWithAllImages(memoryRequirements);
        uint32_t memoryTypeIndex = utils::FindMemoryType(compatibleMemoryTypes,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        //allocate the big memory for all the images
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = totalSize;  // Total size from previous calculations
        allocInfo.memoryTypeIndex = memoryTypeIndex;
        if (vkAllocateMemory(device, &allocInfo, nullptr, &mDeviceMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate large device memory!");
        }
        SET_NAME(mDeviceMemory, VK_OBJECT_TYPE_DEVICE_MEMORY, "GpuTexturesDeviceMemory");
        //now we have to bind the vkImages to sections of the vkDeviceMemory
        currentOffset = 0;
        for (int i = 0; i < vkImages.size(); i++) {
            VkMemoryRequirements memRequirements = memoryRequirements[i];
            // Align the current offset to the required alignment of this image
            currentOffset = (currentOffset + memRequirements.alignment - 1) & ~(memRequirements.alignment - 1);
            // Bind the image to the memory at the aligned offset
            if (vkBindImageMemory(device, vkImages[i], mDeviceMemory, currentOffset) != VK_SUCCESS) {
                throw std::runtime_error("Failed to bind image memory!");
            }
            //copy to the staging buffer
            VkBuffer textureStagingBuffer;
            VkDeviceMemory textureStagingMemory;
            CreateBuffer(imageDataFromFiles[i]->size,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, //will be a source of transfer 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,//memory visible on cpu side 
                textureStagingBuffer, textureStagingMemory, device);
            void* textureDataPtr;
            vkMapMemory(device, textureStagingMemory, 0, imageDataFromFiles[i]->size, 0, &textureDataPtr);
            memcpy(textureDataPtr, imageDataFromFiles[i]->pixels.data(), static_cast<size_t>(imageDataFromFiles[i]->size));
            vkUnmapMemory(device, textureStagingMemory);
            assert(imageDataFromFiles[i]->name != "");
            auto __name = Concatenate(imageDataFromFiles[i]->name, "StagingBuffer");
            SET_NAME(textureStagingBuffer, VK_OBJECT_TYPE_BUFFER, __name.c_str());
            __name = Concatenate(imageDataFromFiles[i]->name, "StagingBufferDeviceMemory");
            SET_NAME(textureStagingMemory, VK_OBJECT_TYPE_DEVICE_MEMORY, __name.c_str());

            //change the image from whatever it is when it's created to a destination of a copy with optimal layout
            TransitionImageLayout(vkImages[i], VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                commandPool, device, graphicsQueue);
            //now that the image can be the dest of a copy, do it
            CopyBufferToImage(textureStagingBuffer, vkImages[i],
                static_cast<uint32_t>(imageDataFromFiles[i]->w), 
                static_cast<uint32_t>(imageDataFromFiles[i]->h));
            //transition it from destination of copy to shader-only.
            TransitionImageLayout(vkImages[i], VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                commandPool, device, graphicsQueue);


            //create the view
            VkImageViewCreateInfo textureViewInfo{};
            textureViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            textureViewInfo.image = vkImages[i];
            textureViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            textureViewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
            textureViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            textureViewInfo.subresourceRange.baseMipLevel = 0;
            textureViewInfo.subresourceRange.levelCount = 1;
            textureViewInfo.subresourceRange.baseArrayLayer = 0;
            textureViewInfo.subresourceRange.layerCount = 1;
            VkImageView textureImageView;
            if (vkCreateImageView(device, &textureViewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
                throw std::runtime_error("failed to create texture image view!");
            }
            __name = Concatenate(imageDataFromFiles[i]->name, "ImageView");
            SET_NAME(textureImageView, VK_OBJECT_TYPE_IMAGE_VIEW, __name.c_str());
            vkDestroyBuffer(device, textureStagingBuffer, nullptr);
            vkFreeMemory(device, textureStagingMemory, nullptr);
            //add to the table
            entities::Image img{
                vkImages[i],
                textureImageView,
                imageDataFromFiles[i]->name
            };
            mImageTable.insert({ utils::Hash(imageDataFromFiles[i]->name), img });
            // Move the offset forward
            currentOffset += memRequirements.size;
        }
        for (auto& i : imageDataFromFiles) {
            delete i;
        }
        imageDataFromFiles.clear();
    }
    ImageService::~ImageService()
    {
        vkFreeMemory(vk::Device::gDevice->GetDevice(), mDeviceMemory, nullptr);
        for (auto& kv : mImageTable) {
            vkDestroyImage(vk::Device::gDevice->GetDevice(), kv.second.mImage, nullptr);
            vkDestroyImageView(vk::Device::gDevice->GetDevice(), kv.second.mImageView, nullptr);
        }
    }
}