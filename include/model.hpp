#include "common.hpp"
#include "descriptors.hpp"

/* TODO:
set correct camera speed, position
declutter draw function
prepare presentation (BD)
*/

// // https://archive.org/embed/GDC2014Bilodeau
struct UniformMeshStruct{
    alignas(16) glm::mat4 cframe = glm::mat4(1.0);
    alignas(4) float roughness = 1.0;
    alignas(4) float metalliness = 0.0;
    alignas(4) int32_t albedo_texture_id = -1; // -1; means no texture
    alignas(4) int32_t material_texture_id = -1; // -1; means no texture
    alignas(4) int32_t mesh_id = -1; // -1; means no mesh, empty
    // R Metallic
    // G Roughness
    // B -
    // A -
};

struct Material{
    std::vector<char> metallic_roughness_pixels;
    std::vector<char> albedo_pixels;
    std::vector<char> normal_pixels;
};

class Mesh{
public:
    std::string name = "mesh";
    glm::mat4 cframe = glm::mat4(1.0);

    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    Material material;
    Region region;

    uint32_t id = 0; // mesh id number
    uint32_t ioffset = 0; // linear memory index offset location
    uint32_t voffset = 0; // linear memory vertex offset location
};

class Node{
public:
    std::string name = "node";
    glm::mat4 cframe = glm::mat4(1.0);
    std::vector<Node> children;
    std::vector<Mesh> meshes;

    struct VulkanObjects{
        VkPipelineLayout *layout;
        VkDescriptorSet *descriptor_set;
        Buffer *indices;
        Buffer *vertices;
    } vk;

    void fill_buffers(Buffer* indices, Buffer* vertices)
    {
        VkDeviceSize size;
        for(Mesh& mesh : meshes){
            size = mesh.indices.size() * sizeof(uint32_t);
            //for(uint32_t i = 0; i < mesh.indices.size(); i++) mesh.indices[i] += mesh.ioffset; // add offset
            indices->fill_memory(mesh.indices.data(), size, mesh.ioffset * sizeof(uint32_t));
            size = mesh.vertices.size() * sizeof(Vertex);
            vertices->fill_memory(mesh.vertices.data(), size, mesh.voffset * sizeof(Vertex));
        }
        for(Node& node : children) node.fill_buffers(indices, vertices);
    }

    void draw(VkCommandBuffer* cmd, VkPipelineLayout *pipeline_layout, Descriptors *descriptors, Buffer *indices, Buffer *vertices, glm::mat4 cframe_offset = glm::mat4(1.0))
    {
        cframe_offset *= this->cframe; 
        for(Mesh& mesh : meshes){

            UniformMeshStruct ums;
            
            ums.cframe = cframe_offset;
            mesh.region = Region(
                cframe_offset * glm::vec4(mesh.region.max.x, mesh.region.max.y, mesh.region.max.z, 1.0),
                cframe_offset * glm::vec4(mesh.region.min.x, mesh.region.min.y, mesh.region.min.z, 1.0)
            );

            msg::error(mesh.region.max - mesh.region.min);
            descriptors->dynamic_uniform_buffer.fill_memory(&ums, sizeof(UniformMeshStruct), DYNAMIC_DESCRIPTOR_SIZE * mesh.id );

            std::array<uint32_t, 1> dbo = { DYNAMIC_DESCRIPTOR_SIZE * mesh.id }; // dynamic buffer offset;
            VkDeviceSize vertex_offset = 0;

            vkCmdBindVertexBuffers(*cmd, 0, 1, &vertices->buffer, &vertex_offset);
            vkCmdBindIndexBuffer(*cmd, indices->buffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdBindDescriptorSets(*cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline_layout, 0, 1, &descriptors->descriptor_sets, 1, dbo.data());
            vkCmdDrawIndexed(*cmd, (uint32_t)mesh.indices.size(), 1, mesh.ioffset, mesh.voffset, 0);
        }

        for(Node& node : children) node.draw(cmd, pipeline_layout, descriptors, indices, vertices, cframe_offset);
    }




    // debug ----------------------------------------------------
    void gap(int n){for(int i=0;i<n;i++)msg::print("  ");}
    void iterate(int depth = 0){
        gap(depth); msg::highlight(this->name);
        
        for(Mesh& mesh : meshes){ 
            gap(depth+1); 
            msg::printl(mesh.name," | ", " ", " | ", mesh.voffset); 
        }

        for(Node& node : children) node.iterate(depth+1);
    }

};

class Model{
private:
    Image albedo_buffer;
    Image material_buffer;
    Buffer indices;
    Buffer vertices;

public:
    uint32_t total_indices_size = 0;
    uint32_t total_vertices_size = 0;
    std::vector<Node> nodes;

    // 1
    void create_buffers(Instance* instance)
    {
        VkDeviceSize size;

        size = sizeof(uint32_t) * this->total_indices_size;
        indices.init(instance);
        indices.create_buffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        size = sizeof(Vertex) * this->total_vertices_size;
        vertices.init(instance);
        vertices.create_buffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        for(Node& node : nodes) node.fill_buffers(&indices, &vertices);
        for(Node& node : nodes) node.iterate();
        msg::success("model buffers created");
    }

    // 2
    void prepare_model(){
        // transform vertices based on node cframe
        // prepare dynamic buffer
        // create mesh materials
    }

    // 3
    void draw(VkCommandBuffer* cmd, VkPipelineLayout *pipeline_layout, Descriptors *descriptors)
    {
        for(Node& node : nodes) node.draw(cmd, pipeline_layout, descriptors, &indices, &vertices);
    }



    // 4
    void destroy(){}
};

