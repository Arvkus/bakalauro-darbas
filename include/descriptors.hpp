#pragma once
#include "common.hpp"
#include "instance.hpp"
#include "memory.hpp"

class Descriptors{
public:

    void init(Instance *instance){
        this->instance = instance;
        create_texture_sampler();
        create_uniform_buffers();
        create_descriptor_set_layout();
        create_descriptor_pool();
    }

    void destroy()
    {
        vkDestroyDescriptorPool(instance->device, descriptor_pool, nullptr);
        vkDestroyDescriptorSetLayout(instance->device, descriptor_set_layout, nullptr);
        
        for(int i = 0; i < uniform_buffers.size(); i++) uniform_buffers[i].destroy();
        vkDestroySampler(instance->device, texture_sampler, nullptr);
    }

    VkSampler texture_sampler;
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool descriptor_pool;
    std::vector<Buffer> uniform_buffers;
    std::vector<VkDescriptorSet> descriptor_sets;

    void bind_diffuse_image(Image *image){ diffuse = image; }
    // specular +
    // roughness +

        void create_descriptor_sets(){
        std::vector<VkDescriptorSetLayout> layouts(instance->surface.image_count, descriptor_set_layout);

        VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.descriptorPool = this->descriptor_pool;
        allocInfo.descriptorSetCount = (uint32_t)instance->surface.image_count;
        allocInfo.pSetLayouts = layouts.data();

        descriptor_sets.resize(instance->surface.image_count);
        if (vkAllocateDescriptorSets(instance->device, &allocInfo, descriptor_sets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        printf("Allocated descriptor sets \n");

        for (size_t i = 0; i < instance->surface.image_count; i++) {

            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = uniform_buffers[i].buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);
            
            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = diffuse->image_view; //texture_image.image_view;
            imageInfo.sampler = texture_sampler;
            
            std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptor_sets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;
            
            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptor_sets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;
            
            vkUpdateDescriptorSets(instance->device, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
        }
        printf("Descriptor sets ready \n");
    }
    
private:
    Image *diffuse;
    Instance *instance;

    void create_texture_sampler()
    {
        VkSamplerCreateInfo samplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        samplerInfo.magFilter = VK_FILTER_LINEAR; // or VK_FILTER_NEAREST
        samplerInfo.minFilter = VK_FILTER_LINEAR; // or VK_FILTER_NEAREST

        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 16;

        samplerInfo.unnormalizedCoordinates = VK_FALSE; // use normalized [0, 1]

        // https://developer.nvidia.com/gpugems/gpugems/part-ii-lighting-and-shadows/chapter-11-shadow-map-antialiasing
        samplerInfo.compareEnable = VK_FALSE; // for shadow mapping
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        if (vkCreateSampler(instance->device, &samplerInfo, nullptr, &texture_sampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }

        printf("created sampler \n");
    }

    void create_uniform_buffers(){

        VkDeviceSize size = sizeof(UniformBufferObject);
        uniform_buffers.resize(instance->surface.image_count);

        for (size_t i = 0; i < instance->surface.image_count; i++) 
        {
            uniform_buffers[i].init(this->instance);
            uniform_buffers[i].create_buffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        }

        printf("created uniform buffers \n");
    }

    void create_descriptor_set_layout()
    {
        // define:
        //  descriptor type
        //  where to use (shader stage)
        //  tell binding for shader program

        VkDescriptorSetLayoutBinding uboLayoutBinding = {};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

        VkDescriptorSetLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        createInfo.bindingCount = (uint32_t)bindings.size();
        createInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(this->instance->device, &createInfo, nullptr, &descriptor_set_layout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    void create_descriptor_pool(){
        std::array<VkDescriptorPoolSize, 2> pool_sizes = {};
        pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pool_sizes[0].descriptorCount = (uint32_t)instance->surface.image_count;
        pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_sizes[1].descriptorCount = (uint32_t)instance->surface.image_count;

        VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolInfo.poolSizeCount = (uint32_t)pool_sizes.size();
        poolInfo.pPoolSizes = pool_sizes.data();
        poolInfo.maxSets = (uint32_t)instance->surface.image_count;

        if (vkCreateDescriptorPool(instance->device, &poolInfo, nullptr, &this->descriptor_pool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }

        printf("created descriptor pool \n");
    }
};