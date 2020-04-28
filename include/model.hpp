#include "common.hpp"
#include "descriptors.hpp"

struct UniformMeshStruct{
    alignas(16) glm::mat4 cframe = glm::mat4(1.0);
    alignas(4) float roughness = 1.0;
    alignas(4) float metalliness = 0.0;
    alignas(4) int32_t albedo_texture_id = -1; // -1; means no texture
    alignas(4) int32_t material_texture_id = -1; // -1; means no texture
    alignas(4) int32_t mesh_id = -1; // -1; means no mesh, empty
    alignas(4) uint32_t index_offset = 0;
    alignas(4) uint32_t vertex_offset = 0;

    // R Metallic
    // G Roughness
    // B -
    // A -
};


// https://archive.org/embed/GDC2014Bilodeau
class Mesh
{
public:
    struct Stage{
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    } stage; 

    std::string name = "null";
    glm::mat4 cframe = glm::mat4(1.0);
    std::vector<Mesh> children;

    uint32_t index_count = 0;
    uint32_t index_offset = 0;
    uint32_t vertex_offset = 0;

    void update_cframe(glm::mat4 cframe_offset = glm::mat4(1.0))
    {
        cframe_offset *= cframe;
        // update buffer here
        for(Mesh& mesh : children) mesh.update_cframe(cframe_offset);
    }

    void draw(const VkCommandBuffer& cmd)
    {
        /*
        for(Mesh& mesh : children) mesh.draw(cmd);
        vkCmdBindVertexBuffers(cmd, 0, 1, &model->vertices.buffer, 0);
        vkCmdBindIndexBuffer(cmd, model->indices.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, index_count, 1, 0, 0, 0);
        */
    }
};

class Model
{
public:
    VkDescriptorSet descriptor_set;
    VkPipelineLayout pipeline_layout;

    Image albedo_buffer;
    Image material_buffer;
    Buffer indices;
    Buffer vertices;

    std::vector<Mesh> meshes;

    void draw(const VkCommandBuffer& cmd)
    {
        uint32_t index = 0;
        std::array<uint32_t, 1> dbo = { DYNAMIC_DESCRIPTOR_SIZE * index }; // dynamic buffer offsets

        for(Mesh& mesh : meshes) mesh.update_cframe();
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,  &descriptor_set, 1, dbo.data());
        for(Mesh& mesh : meshes) mesh.draw(cmd);
    }

    void destroy(){}
};

