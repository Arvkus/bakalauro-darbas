
#pragma once
#include "common.hpp"
#include "instance.hpp"
// https://stackoverflow.com/questions/42595473/correct-usage-of-unique-ptr-in-class-member

class Memory{
public:

    static void init(Instance *instance)
    {
         Memory::instance = instance;
    }

protected:
    static const inline Instance *instance;
    
    
    VkDeviceMemory memory;

    uint32_t find_memory_type(uint32_t typeBits, VkMemoryPropertyFlags requiredProperties) {

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

//-------------------------------------------

class MemoryBuffer : Memory
{
private:
    VkBuffer buffer;

public:
    
    MemoryBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties){

        VkBufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.size = size;
        createInfo.usage = usage;

        if(vkCreateBuffer(this->instance->device, &createInfo, nullptr, &buffer) != VK_SUCCESS){
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements = {};
        vkGetBufferMemoryRequirements(this->instance->device, buffer, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, properties);

        if(vkAllocateMemory(this->instance->device, &allocInfo, nullptr, &memory) != VK_SUCCESS){
            throw std::runtime_error("failed to allocate memory!");
        };

        vkBindBufferMemory(this->instance->device, buffer, memory, 0);
    }
};

//-------------------------------------------

class MemoryImage : Memory
{
private:
    VkImage image;
public:
};
