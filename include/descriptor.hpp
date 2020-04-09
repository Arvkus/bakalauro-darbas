#pragma once
#include "common.hpp"
#include "instance.hpp"
#include "memory.hpp"

class Descriptor{
public:
    std::vector<Buffer> uniform_buffers;
    VkDescriptorPool descriptor_pool;
    std::vector<VkDescriptorSet> descriptor_sets;
    VkSampler texture_sampler;

private:
    Instance *instance;
};