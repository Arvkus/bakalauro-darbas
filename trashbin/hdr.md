
Help me to understand dynamic uniforms

Lets say I have Material struct with 1 variable - roughness, and I need different value for every draw call (object).

```cpp
struct Material {
	float roughness;
};
```

With maximum object count I need to create buffer with the size of:
```cpp
VkDeviceSize size = sizeof(Material) * MAX_OBJECT_COUNT;
```

and to update buffer data for certain object use this offset:
```cpp
uint32_t offset =  sizeof(Material) * object_index;
```

for simplicity I have only 1 object and to use material data for shader uniform
```
std::array<uint32_t, 1> dynamic_offsets = {0};
vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, descriptor_sets, 1, dynamic_offsets.data())
```




My current layout bindings look like this:
```cpp
dynamicMaterial.binding = 0;
dynamicMaterial.descriptorCount = 1;
dynamicMaterial.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; // !dyn
dynamicMaterial.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

mvp.binding = 1;
mvp.descriptorCount = 1;
mvp.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
mvp.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

samplerLayoutBinding.binding = 2;
samplerLayoutBinding.descriptorCount = 1;
samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

skyboxLayoutBinding.binding = 3;
skyboxLayoutBinding.descriptorCount = 1;
skyboxLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
skyboxLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        
```
