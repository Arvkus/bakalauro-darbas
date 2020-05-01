#pragma once
#include "common.hpp"
#include "instance.hpp"
#include "memory.hpp"

class Descriptors{
public:

    void init(Instance *instance){
        this->instance = instance;
        create_texture_sampler();
        create_texture_array();
        create_uniform_buffers();
        create_descriptor_set_layout();
        create_descriptor_pool();
    }

    void destroy()
    {
        vkDestroyDescriptorPool(instance->device, descriptor_pool, nullptr);
        vkDestroyDescriptorSetLayout(instance->device, descriptor_set_layout, nullptr);

        
        for(uint32_t i = 0; i < MAX_IMAGES; i++){
            color_image_pool[i].destroy();
            material_image_pool[i].destroy();
        }
        
        uniform_buffer.destroy();
        dynamic_uniform_buffer.destroy();
        vkDestroySampler(instance->device, texture_sampler, nullptr);
    }

    VkSampler texture_sampler;
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool descriptor_pool;
    Buffer uniform_buffer;
    Buffer dynamic_uniform_buffer;
    VkDescriptorSet descriptor_sets;

    std::array<Image, MAX_IMAGES> color_image_pool;
    std::array<Image, MAX_IMAGES> material_image_pool;

    void create_texture_array()
    {
        // load default img
        int width = 0, height = 0, channel = 0;
        stbi_uc* pixels = stbi_load("textures/placeholder.png", &width, &height, &channel, STBI_rgb_alpha);
        if(!pixels) throw std::runtime_error("failed to load texture image!");

        // resize default img
        stbi_uc* new_pixels = (stbi_uc*) malloc(MAX_IMAGE_SIZE * MAX_IMAGE_SIZE * 4);
        stbir_resize_uint8(pixels, width , height , 0, new_pixels, MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, 0, 4);

        for(uint32_t i = 0; i < MAX_IMAGES; i++){
            color_image_pool[i].init(this->instance);
            color_image_pool[i].create_image(MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
            color_image_pool[i].fill_memory(MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, 4, new_pixels);
            color_image_pool[i].create_image_view(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

            material_image_pool[i].init(this->instance);
            material_image_pool[i].create_image(MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
            material_image_pool[i].fill_memory(MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, 4, new_pixels);
            material_image_pool[i].create_image_view(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
        }

        stbi_image_free(new_pixels);
        stbi_image_free(pixels);
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

        VkDescriptorBufferInfo materialInfo = {};
        materialInfo.buffer = dynamic_uniform_buffer.buffer;
        materialInfo.offset = 0;
        materialInfo.range = DYNAMIC_DESCRIPTOR_SIZE; // sizeof(Material);

        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = uniform_buffer.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);
        
        VkDescriptorImageInfo imageInfos[MAX_IMAGES];
        for (uint32_t i = 0; i < MAX_IMAGES; ++i)
        {
            imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos[i].imageView = color_image_pool[i].image_view; // this->diffuse->image_view; //
            imageInfos[i].sampler = texture_sampler;
        }


        VkDescriptorImageInfo materialInfos[MAX_IMAGES];
        for (uint32_t i = 0; i < MAX_IMAGES; ++i)
        {
            materialInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            materialInfos[i].imageView = material_image_pool[i].image_view; // this->diffuse->image_view; //
            materialInfos[i].sampler = texture_sampler;
        }

        VkDescriptorImageInfo enviromentInfo = {};
        enviromentInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        enviromentInfo.imageView = enviroment->image_view; 
        enviromentInfo.sampler = texture_sampler;

        std::array<VkWriteDescriptorSet, 5> descriptorWrites = {};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptor_sets;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; // !dyn
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &materialInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptor_sets;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &bufferInfo;
        
        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = descriptor_sets;
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[2].descriptorCount = MAX_IMAGES; 
        descriptorWrites[2].pImageInfo = imageInfos;
        
        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet = descriptor_sets;
        descriptorWrites[3].dstBinding = 3;
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[3].descriptorCount = 1;
        descriptorWrites[3].pImageInfo = &enviromentInfo;

        descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[4].dstSet = descriptor_sets;
        descriptorWrites[4].dstBinding = 4;
        descriptorWrites[4].dstArrayElement = 0;
        descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[4].descriptorCount = MAX_IMAGES; 
        descriptorWrites[4].pImageInfo = materialInfos;

        
        
        vkUpdateDescriptorSets(instance->device, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
        
        printf("Descriptor sets ready \n");
    }
    
private:

    Image *enviroment;
    //-- PBR textures --
    Image *diffuse; // color
    Image *normal;  // direction
    Image *orm;
    /*
    Image *occlusion; // R
    Image *rougness;  // G
    Image *metalic;   // B
    */
    //------------------
    
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

    void create_uniform_buffers(){
        
        VkDeviceSize size;
        
        size = sizeof(UniformBufferObject);
        uniform_buffer.init(this->instance);
        uniform_buffer.create_buffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
        size = DYNAMIC_DESCRIPTOR_SIZE * MAX_OBJECTS; // sizeof(Material) 
        dynamic_uniform_buffer.init(this->instance);
        dynamic_uniform_buffer.create_buffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
        printf("created uniform buffers \n");
    }

    void create_descriptor_set_layout()
    {
        // define:
        //  descriptor type
        //  where to use (shader stage)
        //  tell binding for shader program

        VkDescriptorSetLayoutBinding dynamicMaterial = {};
        dynamicMaterial.binding = 0;
        dynamicMaterial.descriptorCount = 1;
        dynamicMaterial.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; // !dyn
        dynamicMaterial.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding uboLayoutBinding = {};
        uboLayoutBinding.binding = 1;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
        samplerLayoutBinding.binding = 2;
        samplerLayoutBinding.descriptorCount = MAX_IMAGES; // array of images
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        
        VkDescriptorSetLayoutBinding skyboxLayoutBinding = {};
        skyboxLayoutBinding.binding = 3;
        skyboxLayoutBinding.descriptorCount = 1;
        skyboxLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        skyboxLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        
        VkDescriptorSetLayoutBinding materialLayoutBinding = {};
        materialLayoutBinding.binding = 4;
        materialLayoutBinding.descriptorCount = MAX_IMAGES; // array of images
        materialLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        materialLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
       
        
        std::array<VkDescriptorSetLayoutBinding, 5> bindings = {
            dynamicMaterial,
            uboLayoutBinding, 
            samplerLayoutBinding, 
            skyboxLayoutBinding,
            materialLayoutBinding
        };

        VkDescriptorSetLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        createInfo.bindingCount = (uint32_t)bindings.size();
        createInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(this->instance->device, &createInfo, nullptr, &descriptor_set_layout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
         printf("Created descriptor set layouts \n");
    }

    void create_descriptor_pool()
    {
        std::array<VkDescriptorPoolSize, 5> pool_sizes = {};
        pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; // !dyn
        pool_sizes[0].descriptorCount = 1;
        pool_sizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pool_sizes[1].descriptorCount = 1;
        pool_sizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_sizes[2].descriptorCount = 1;
        pool_sizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_sizes[3].descriptorCount = 1;
        pool_sizes[4].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_sizes[4].descriptorCount = 1;

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


class Descriptor
{
private:
    static inline Instance* instance;

    struct DescriptorInfo
    {
        VkDeviceSize binding;
        VkDescriptorType type;
        VkShaderStageFlags stage;
        uint32_t count;
        uint32_t range = 0;
        Image* image;
        Buffer* buffer;
    };

    std::vector<DescriptorInfo> infos;

    VkSampler texture_sampler;
    VkDescriptorPool descriptor_pool;

    //----------------------------------------------------

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

    //----------------------------------------------------

    void create_descriptor_pool()
    {
        std::vector<VkDescriptorPoolSize> pool_sizes(infos.size());
        for(int i = 0; i < infos.size(); i++){
            pool_sizes[i].type = infos[i].type;
            pool_sizes[i].descriptorCount = 1;
        }

        VkDescriptorPoolCreateInfo ci = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        ci.poolSizeCount = (uint32_t)pool_sizes.size();
        ci.pPoolSizes = pool_sizes.data();
        ci.maxSets = 1;

        if (vkCreateDescriptorPool(instance->device, &ci, nullptr, &this->descriptor_pool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }

        printf("Created descriptor pool \n");
    }

    //----------------------------------------------------

    void create_descriptor_set_layout()
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings(infos.size());
        for(int i = 0; i < infos.size(); i++){
            bindings[i].binding = infos[i].binding;
            bindings[i].descriptorCount = infos[i].count;
            bindings[i].descriptorType = infos[i].type;
            bindings[i].stageFlags = infos[i].stage;
        }

        VkDescriptorSetLayoutCreateInfo ci = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        ci.bindingCount = (uint32_t)bindings.size();
        ci.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(this->instance->device, &ci, nullptr, &descriptor_set_layout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
        printf("Created descriptor set layouts \n");
    }

    //----------------------------------------------------

    void create_descriptor_set()
    {
        VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.descriptorPool = this->descriptor_pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptor_set_layout;

        VkResult result = vkAllocateDescriptorSets(instance->device, &allocInfo, &descriptor_set);
        if (result != VK_SUCCESS) {
            msg::error(result);
            throw std::runtime_error("failed to allocate descriptor sets!");
        }
    }

    void fill_descriptor_sets()
    {
        std::vector<VkWriteDescriptorSet> writes(infos.size());

        for(uint32_t i = 0; i < infos.size(); i++)
        {
            if(infos[i].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER){
                VkDescriptorImageInfo imageInfo = {};
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo.imageView = infos[i].image->image_view; 
                imageInfo.sampler = texture_sampler;

                writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writes[i].dstSet = this->descriptor_set;
                writes[i].dstBinding = 3;
                writes[i].dstArrayElement = 0;
                writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                writes[i].descriptorCount = 1;
                writes[i].pImageInfo = &imageInfo;
            }else{
                VkDescriptorBufferInfo bufferInfo = {};
                bufferInfo.buffer = infos[i].buffer->buffer;
                bufferInfo.offset = 0;
                bufferInfo.range = infos[i].range;

                writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writes[i].dstSet = this->descriptor_set;
                writes[i].dstBinding = 0;
                writes[i].dstArrayElement = 0;
                writes[i].descriptorType = infos[i].type; // !dyn
                writes[i].descriptorCount = 1;
                writes[i].pBufferInfo = &bufferInfo;
            }
        }
        
        vkUpdateDescriptorSets(instance->device, (uint32_t)infos.size(), writes.data(), 0, nullptr);
        printf("Descriptor sets ready \n");
    }

    //----------------------------------------------------
    
public:
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorSet descriptor_set;
    
    static void init(Instance *instance){Descriptor::instance = instance;}

    void add(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage, uint32_t count = 1)
    {
        DescriptorInfo info;
        info.binding = binding;
        info.type = type;
        info.stage = stage;
        info.count = count;
        infos.push_back(info);
    }

    void bind_buffer(uint32_t binding, Buffer* buffer, uint32_t range){
        for(DescriptorInfo& info : infos){
            if(info.binding == binding){
                info.buffer = buffer; break;
            }
        }
    }

    void bind_image(uint32_t binding, Image* image, uint32_t range){
        for(DescriptorInfo& info : infos){
            if(info.binding == binding){
                info.image = image; break;
            }
        }
    }

    void create()
    {
        create_descriptor_pool();
        create_descriptor_set_layout();
        create_descriptor_set();
        fill_descriptor_sets();
    }
};


