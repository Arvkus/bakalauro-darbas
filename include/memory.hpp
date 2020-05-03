
#pragma once
#include "common.hpp"
#include "instance.hpp"

// https://stackoverflow.com/questions/42595473/correct-usage-of-unique-ptr-in-class-member
//--------------------------------------------------------------------------------------------

class MemoryObject
{
public:
    VkDeviceMemory memory;
    

    void init(Instance *instance){ this->instance = instance; }

protected:
    Instance *instance;

    uint32_t find_memory_type(uint32_t typeBits, VkMemoryPropertyFlags requiredProperties) 
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(instance->physical_device, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            const VkMemoryPropertyFlags& supportedProperties = memProperties.memoryTypes[i].propertyFlags;
            //std::cout<< "memory search: " << std::bitset<16>(typeBits) << " (" << typeBits << ") & " << std::bitset<16>(1 << i) << 
            //" && " << std::bitset<16>(properties) << " & " << std::bitset<16>(supportedProperties) <<std::endl;
            if (typeBits & (1 << i) && (supportedProperties & requiredProperties) == requiredProperties) {
                return i;
            }
        }
        throw std::runtime_error("failed to find suitable memory type!");
    }
};

//--------------------------------------------------------------------------------------------

class Buffer : public MemoryObject{
public:
    VkBuffer buffer;

    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    {
        VkBufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.size = size;
        createInfo.usage = usage;

        if(vkCreateBuffer(instance->device, &createInfo, nullptr, &this->buffer) != VK_SUCCESS){
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements = {};
        vkGetBufferMemoryRequirements(instance->device, this->buffer, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, properties);

        if(vkAllocateMemory(instance->device, &allocInfo, nullptr, &this->memory) != VK_SUCCESS){
            throw std::runtime_error("failed to allocate memory for buffer!");
        };

        vkBindBufferMemory(instance->device, this->buffer, this->memory, 0);
    }

    void fill_memory(const void* source, VkDeviceSize size, uint32_t offset = 0)
    {
        void* data; // memory location to where to copy
        vkMapMemory(instance->device, this->memory, offset, size, 0, &data);
        memcpy(data, source, (size_t)size); // destination, source, size
        vkUnmapMemory(instance->device, this->memory);

        
    }

    void destroy(){
        vkFreeMemory(instance->device, memory, nullptr);
        vkDestroyBuffer(instance->device, buffer, nullptr);
    }
};

//--------------------------------------------------------------------------------------------

class Image : public MemoryObject{
public:

    VkImage image;
    VkImageView image_view;
    //VkImageLayout current_layout;

    void create_image(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, uint32_t layers = 1)
    {
        //this->current_layout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkImageCreateInfo ci = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        ci.imageType = VK_IMAGE_TYPE_2D;
        ci.extent.width = width;
        ci.extent.height = height;
        ci.extent.depth = 1;
        ci.format = format;
        ci.usage = usage;
        ci.tiling = VK_IMAGE_TILING_OPTIMAL; // (or VK_IMAGE_TILING_LINEAR) efficient texel tiling
        ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // this->current_layout;
        ci.arrayLayers = layers;
        ci.mipLevels = 1;
        ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ci.samples = VK_SAMPLE_COUNT_1_BIT;
        
        if(vkCreateImage(instance->device, &ci, nullptr, &this->image) != VK_SUCCESS){
            throw std::runtime_error("failed to create image!");
        };

        VkMemoryRequirements mem_requirements;
        vkGetImageMemoryRequirements(instance->device, this->image, &mem_requirements);

        VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);

        if(vkAllocateMemory(instance->device, &alloc_info, nullptr, &this->memory) != VK_SUCCESS){
            throw std::runtime_error("failed to allocate memory for image!");
        };

        vkBindImageMemory(instance->device, this->image, this->memory, 0);
    }


    void fill_memory(uint32_t width, uint32_t height, uint32_t channel, const void *source, uint32_t img_index = 0)
    {   
        VkDeviceSize size = width * height * channel;
        Buffer stage;
        stage.init(instance);
        stage.create_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        stage.fill_memory(source, size); // move image to prepared buffer (RAM)

        VkImageSubresourceRange range = {};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = img_index;
        range.layerCount = 1;
  
        transition_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);
        copy_buffer_to_image(stage.buffer, this->image, width, height, img_index); // move image to VRAM
        transition_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);

        stage.destroy();
    }



    void create_image_view(VkFormat format, VkImageAspectFlags aspectFlags)
    {
        VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format; // VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = aspectFlags; // VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(instance->device, &viewInfo, nullptr, &image_view) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }

    }

    VkImageView return_image_view(uint32_t img_index)
    {
        VkImageView view;
        VkImageViewCreateInfo ci = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        ci.image = image;
        ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ci.format = VK_FORMAT_R8G8B8A8_SRGB;
        ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ci.subresourceRange.baseMipLevel = 0;
        ci.subresourceRange.levelCount = 1;
        ci.subresourceRange.baseArrayLayer = img_index;
        ci.subresourceRange.layerCount = 1;

        if (vkCreateImageView(instance->device, &ci, nullptr, &view) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }
        
        return view;
    }

    void destroy()
    {
        vkDestroyImageView(instance->device, image_view, nullptr);
        vkDestroyImage(instance->device, image, nullptr);
        vkFreeMemory(instance->device, memory, nullptr);
    }

    //----------------------------------------------------------------------------------------------
private:

    void transition_layout(VkImageLayout old_layout, VkImageLayout new_layout, VkImageSubresourceRange range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}){
        VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;

        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = image;
        barrier.subresourceRange = range;
        /*
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        */

        //-------------------------------------------------------------

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } 
        else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } 
        else 
        {
            throw std::invalid_argument("unsupported layout transition!");
        }

        //-------------------------------------------------------------

        VkCommandBuffer commandBuffer = this->instance->begin_single_use_command();


        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        this->instance->end_single_use_command(commandBuffer);
        //current_layout = new_layout;
    }

    void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t img_index = 0) 
    {   
        VkDeviceSize memory_offset = width * height * 4 * img_index;

        VkBufferImageCopy region = {};
        region.bufferOffset = 0; // memory_offset
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = img_index;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        VkCommandBuffer commandBuffer = this->instance->begin_single_use_command();

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

        this->instance->end_single_use_command(commandBuffer);
    }

};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
