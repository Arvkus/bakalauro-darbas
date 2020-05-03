#pragma once
#include "common.hpp"
#include "instance.hpp"
#include "memory.hpp"

class Descriptors{
public:

    void init(Instance *instance){
        this->instance = instance;
        create_texture_sampler();
        create_images();
        create_uniform_buffers();
        create_descriptor_set_layout();
        create_descriptor_pool();
    }

    void destroy()
    {
        vkDestroyDescriptorPool(instance->device, descriptor_pool, nullptr);
        vkDestroyDescriptorSetLayout(instance->device, descriptor_set_layout, nullptr);

        for(uint32_t i = 0; i < MAX_IMAGES; i++){
            vkDestroyImageView(instance->device, albedo_image_views[i], nullptr);
            vkDestroyImageView(instance->device, normal_image_views[i], nullptr);
            vkDestroyImageView(instance->device, material_image_views[i], nullptr);
            vkDestroyImageView(instance->device, emission_image_views[i], nullptr);
        }
        
        view_buffer.destroy();
        properties_buffer.destroy();
        dynamic_uniform_buffer.destroy();
        vkDestroySampler(instance->device, texture_sampler, nullptr);
    }

    VkSampler texture_sampler;
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_sets;

    Buffer view_buffer;
    Buffer properties_buffer;
    Buffer dynamic_uniform_buffer;

    Image *enviroment;
    Image albedo; 
    Image normal;  
    Image material;
    Image emission;

    std::array<VkImageView, MAX_IMAGES> albedo_image_views;
    std::array<VkImageView, MAX_IMAGES> normal_image_views;
    std::array<VkImageView, MAX_IMAGES> material_image_views;
    std::array<VkImageView, MAX_IMAGES> emission_image_views;

    void create_images()
    {
        // load default img
        int width = 0, height = 0, channel = 0;
        stbi_uc* pixels = stbi_load("textures/placeholder.png", &width, &height, &channel, STBI_rgb_alpha);
        if(!pixels) throw std::runtime_error("failed to load texture image!");

        // resize default img
        stbi_uc* new_pixels = (stbi_uc*) malloc(MAX_IMAGE_SIZE * MAX_IMAGE_SIZE * 4);
        stbir_resize_uint8(pixels, width , height , 0, new_pixels, MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, 0, 4);

        //------------------------------------------------

        this->albedo.init(this->instance);
        this->normal.init(this->instance);
        this->material.init(this->instance);
        this->emission.init(this->instance);

        albedo.create_image(MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, MAX_IMAGES);
        normal.create_image(MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, MAX_IMAGES);
        material.create_image(MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, MAX_IMAGES);
        emission.create_image(MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, MAX_IMAGES);

        for(uint32_t i = 0; i < MAX_IMAGES; i++)
        {
            
            albedo.fill_memory(MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, 4, new_pixels, i);
            normal.fill_memory(MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, 4, new_pixels, i);
            material.fill_memory(MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, 4, new_pixels, i);
            emission.fill_memory(MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, 4, new_pixels, i);
            
            albedo_image_views[i] = albedo.return_image_view(i);
            normal_image_views[i] = normal.return_image_view(i);
            material_image_views[i] = material.return_image_view(i);
            emission_image_views[i] = emission.return_image_view(i);
        }

        stbi_image_free(new_pixels);
        stbi_image_free(pixels);
    }

    void create_uniform_buffers(){
        
        VkDeviceSize size;
        
        size = sizeof(UniformCameraStruct);
        view_buffer.init(this->instance);
        view_buffer.create_buffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        size = sizeof(UniformPropertiesStruct);
        properties_buffer.init(this->instance);
        properties_buffer.create_buffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
        size = DYNAMIC_DESCRIPTOR_SIZE * MAX_OBJECTS; // sizeof(Material) 
        dynamic_uniform_buffer.init(this->instance);
        dynamic_uniform_buffer.create_buffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
        printf("created uniform buffers \n");
    }

    void bind_enviroment_image(Image *image){ enviroment = image; }

    void create_descriptor_sets()
    {
        VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.descriptorPool = this->descriptor_pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptor_set_layout;

        VkResult result = vkAllocateDescriptorSets(instance->device, &allocInfo, &descriptor_sets);
        if (result != VK_SUCCESS) {
            msg::error(result);
            throw std::runtime_error("failed to allocate descriptor sets!");
        }
        //---------------------------------------------------------

        VkDescriptorBufferInfo dbi0 = {}; // view
        dbi0.buffer = view_buffer.buffer;
        dbi0.offset = 0;
        dbi0.range = sizeof(UniformCameraStruct);

        VkDescriptorBufferInfo dbi1 = {}; // props
        dbi1.buffer = view_buffer.buffer;
        dbi1.offset = 0;
        dbi1.range = sizeof(UniformPropertiesStruct);

        VkDescriptorBufferInfo dbi2 = {}; // mesh
        dbi2.buffer = dynamic_uniform_buffer.buffer;
        dbi2.offset = 0;
        dbi2.range = DYNAMIC_DESCRIPTOR_SIZE; // sizeof(Material);

        VkDescriptorImageInfo enviroment_info = {}; // enviroment
        enviroment_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        enviroment_info.imageView = enviroment->image_view;
        enviroment_info.sampler = texture_sampler;

        VkDescriptorImageInfo albedo_info[MAX_IMAGES];
        VkDescriptorImageInfo normal_info[MAX_IMAGES];
        VkDescriptorImageInfo material_info[MAX_IMAGES];
        VkDescriptorImageInfo emission_info[MAX_IMAGES];

        for (uint32_t i = 0; i < MAX_IMAGES; ++i)
        {
            albedo_info[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            albedo_info[i].imageView = albedo_image_views[i];
            albedo_info[i].sampler = texture_sampler;

            normal_info[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            normal_info[i].imageView = normal_image_views[i];
            normal_info[i].sampler = texture_sampler;

            material_info[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            material_info[i].imageView = material_image_views[i];
            material_info[i].sampler = texture_sampler;

            emission_info[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            emission_info[i].imageView = emission_image_views[i];
            emission_info[i].sampler = texture_sampler;
        }

        //---------------------------------------------------------

        std::array<VkWriteDescriptorSet, 8> descriptorWrites = {};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; // view
        descriptorWrites[0].dstSet = descriptor_sets;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &dbi0;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; // properties
        descriptorWrites[1].dstSet = descriptor_sets;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &dbi1;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; // mesh
        descriptorWrites[2].dstSet = descriptor_sets;
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; 
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = &dbi2;

        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; // enviroment
        descriptorWrites[3].dstSet = descriptor_sets;
        descriptorWrites[3].dstBinding = 3;
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[3].descriptorCount = 1; 
        descriptorWrites[3].pImageInfo = &enviroment_info;
        
        descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; // albedo
        descriptorWrites[4].dstSet = descriptor_sets;
        descriptorWrites[4].dstBinding = 4;
        descriptorWrites[4].dstArrayElement = 0;
        descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[4].descriptorCount = MAX_IMAGES; 
        descriptorWrites[4].pImageInfo = albedo_info;

        descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; // normal
        descriptorWrites[5].dstSet = descriptor_sets;
        descriptorWrites[5].dstBinding = 5;
        descriptorWrites[5].dstArrayElement = 0;
        descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[5].descriptorCount = MAX_IMAGES; 
        descriptorWrites[5].pImageInfo = normal_info;

        descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; // material
        descriptorWrites[6].dstSet = descriptor_sets;
        descriptorWrites[6].dstBinding = 6;
        descriptorWrites[6].dstArrayElement = 0;
        descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[6].descriptorCount = MAX_IMAGES; 
        descriptorWrites[6].pImageInfo = material_info;

        descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; // emission
        descriptorWrites[7].dstSet = descriptor_sets;
        descriptorWrites[7].dstBinding = 7;
        descriptorWrites[7].dstArrayElement = 0;
        descriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[7].descriptorCount = MAX_IMAGES; 
        descriptorWrites[7].pImageInfo = emission_info;
        

        //---------------------------------------------------------

        vkUpdateDescriptorSets(instance->device, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
        
        printf("Descriptor sets ready \n");
    }
    
private:
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
    }

    void create_descriptor_set_layout()
    {
        // view, properties, mesh, enviroment, albedo, normal, material, emission
        std::array<VkDescriptorSetLayoutBinding, 8> bindings;
        for(uint32_t i =0; i < bindings.size(); i++) bindings[i] = {};

        bindings[0].binding = 0; // view
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        bindings[1].binding = 1; // properties
        bindings[1].descriptorCount = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[2].binding = 2; // mesh
        bindings[2].descriptorCount = 1;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        bindings[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        
        bindings[3].binding = 3; // enviroment
        bindings[3].descriptorCount = 1;
        bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        
        
        bindings[4].binding = 4; // albedo
        bindings[4].descriptorCount = MAX_IMAGES;
        bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        //bindings[4].pImmutableSamplers

        bindings[5].binding = 5; // normal
        bindings[5].descriptorCount = MAX_IMAGES;
        bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[6].binding = 6; // material
        bindings[6].descriptorCount = MAX_IMAGES;
        bindings[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[6].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[7].binding = 7; // emission
        bindings[7].descriptorCount = MAX_IMAGES;
        bindings[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[7].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo ci = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        ci.bindingCount = (uint32_t)bindings.size();
        ci.pBindings = bindings.data();


        if (vkCreateDescriptorSetLayout(this->instance->device, &ci, nullptr, &descriptor_set_layout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }

        printf("Created descriptor set layouts \n");
    }

    void create_descriptor_pool()
    {
        std::array<VkDescriptorPoolSize, 8> pool_sizes = {};
        pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // view
        pool_sizes[0].descriptorCount = 1;
        pool_sizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // properties
        pool_sizes[1].descriptorCount = 1;
        pool_sizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; // mesh
        pool_sizes[2].descriptorCount = 1;
        pool_sizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // enviroment
        pool_sizes[3].descriptorCount = 1;
        
        pool_sizes[4].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // albedo
        pool_sizes[4].descriptorCount = 1;
        pool_sizes[5].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // normal
        pool_sizes[5].descriptorCount = 1;
        pool_sizes[6].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // material
        pool_sizes[6].descriptorCount = 1;
        pool_sizes[7].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // emission
        pool_sizes[7].descriptorCount = 1;
        

        VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolInfo.poolSizeCount = (uint32_t)pool_sizes.size();
        poolInfo.pPoolSizes = pool_sizes.data();
        poolInfo.maxSets = 1;

        if (vkCreateDescriptorPool(instance->device, &poolInfo, nullptr, &this->descriptor_pool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }

        printf("Created descriptor pool \n");
    }
};

/*
        md.add(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1); // view
        md.add(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1); // properties
        md.add(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1); // mesh
        md.add(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1); // enviroment
        md.add(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1); // albedo
        md.add(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1); // normal
        md.add(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1); // material
        md.add(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1); // emission

*/