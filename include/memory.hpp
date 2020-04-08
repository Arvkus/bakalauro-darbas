
#pragma once
#include "common.hpp"
#include "instance.hpp"

// https://stackoverflow.com/questions/42595473/correct-usage-of-unique-ptr-in-class-member

//-------------------------------------------

struct MemoryBuffer{
    VkDeviceMemory memory;
    VkBuffer buffer;
};

struct MemoryImage{
    VkDeviceMemory memory;
    VkImage image;
};

//-------------------------------------------

class MemoryManager{
public:

    void init(Instance *instance)
    {
         this->instance = instance;
    }

    MemoryBuffer create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    {
        MemoryBuffer memory_object = {};

        VkBufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.size = size;
        createInfo.usage = usage;

        if(vkCreateBuffer(this->instance->device, &createInfo, nullptr, &memory_object.buffer) != VK_SUCCESS){
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements = {};
        vkGetBufferMemoryRequirements(this->instance->device, memory_object.buffer, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, properties);

        if(vkAllocateMemory(this->instance->device, &allocInfo, nullptr, &memory_object.memory) != VK_SUCCESS){
            throw std::runtime_error("failed to allocate memory for buffer!");
        };

        vkBindBufferMemory(this->instance->device, memory_object.buffer, memory_object.memory, 0);
        return memory_object;
    }

    //-----------------------------------------------------------------------------
    
    MemoryImage create_image(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage)
    {
        MemoryImage memory_object;

        VkImageCreateInfo ci = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        ci.imageType = VK_IMAGE_TYPE_2D;
        ci.extent.width = width;
        ci.extent.height = height;
        ci.extent.depth = 1;
        ci.format = format;
        ci.usage = usage;
        ci.tiling = VK_IMAGE_TILING_OPTIMAL; // (or VK_IMAGE_TILING_LINEAR) efficient texel tiling
        ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        ci.arrayLayers = 1;
        ci.mipLevels = 1;
        ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ci.samples = VK_SAMPLE_COUNT_1_BIT;
        
        if(vkCreateImage(this->instance->device, &ci, nullptr, &memory_object.image) != VK_SUCCESS){
            throw std::runtime_error("failed to create image!");
        };

        VkMemoryRequirements mem_requirements;
        vkGetImageMemoryRequirements(this->instance->device, memory_object.image, &mem_requirements);

        VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);

        if(vkAllocateMemory(this->instance->device, &alloc_info, nullptr, &memory_object.memory) != VK_SUCCESS){
            throw std::runtime_error("failed to allocate memory for image!");
        };

        vkBindImageMemory(this->instance->device, memory_object.image, memory_object.memory, 0);
        return memory_object;
    }

    //-----------------------------------------------------------------------------

    void fill_memory_buffer(MemoryBuffer object, const void* source, VkDeviceSize size)
    {
         // fill memory
        void* data; // memory location to where to copy
        vkMapMemory(instance->device, object.memory, 0, size, 0, &data);
        memcpy(data, source, (size_t)size); // destination, source, size
        vkUnmapMemory(instance->device, object.memory);
    }

    void fill_memory_image(MemoryImage object, const void* source, VkDeviceSize size)
    {
         // fill memory
        void* data; // memory location to where to copy
        vkMapMemory(instance->device, object.memory, 0, size, 0, &data);
        memcpy(data, source, (size_t)size); // destination, source, size
        vkUnmapMemory(instance->device, object.memory);
    }
    

private: 
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

