#include "common.hpp"
#include "descriptors.hpp"

// https://archive.org/embed/GDC2014Bilodeau

struct UniformMeshStruct{
    alignas(16) glm::mat4 cframe = glm::mat4(1.0);
    alignas(4) float roughness = 1.0;
    alignas(4) float metalliness = 0.0;
    alignas(4) int32_t albedo_texture_id = -1; // -1; means no texture
    alignas(4) int32_t metal_roughness_texture_id = -1; // -1; means no texture
    alignas(16) glm::vec3 base_color = glm::vec3(1.0);
    // R Metallic
    // G Roughness
    // B -
    // A -
};

struct Material{
    std::vector<char> metallic_roughness_pixels;
    std::vector<char> albedo_pixels;
    std::vector<char> normal_pixels;

    int32_t metallic_roughness_texture_id = -1;
    int32_t albedo_texture_id = -1;
    int32_t normal_texture_id = -1;
    
    float roughness = 1.0;
    float metalliness = 0.0;
    glm::vec3 base_color = glm::vec3(1.0);
};

struct MeshDrawInfo{
    uint32_t id = 0;
	uint32_t vertex_offset = 0;
	uint32_t index_offset = 0;
	uint32_t index_count = 0;
	Region region;
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

    void get_draw_info(std::vector<MeshDrawInfo>& infos, glm::mat4 cframe_offset = glm::mat4(1.0))
    {
        cframe_offset *= this->cframe; 

        for(Mesh& mesh : meshes){ 
            MeshDrawInfo info;
            info.id = mesh.id;
            info.index_offset = mesh.ioffset;
            info.vertex_offset = mesh.voffset;
            info.index_count = mesh.indices.size();
            info.region = Region(
                cframe_offset * glm::vec4(mesh.region.max.x, mesh.region.max.y, mesh.region.max.z, 1.0),
                cframe_offset * glm::vec4(mesh.region.min.x, mesh.region.min.y, mesh.region.min.z, 1.0)
            );
            infos.push_back(info);
        }

        for(Node& node : children) node.get_draw_info(infos, cframe_offset);
    }

    void update_dynamic_buffer(Descriptors *descriptors, glm::mat4 cframe_offset = glm::mat4(1.0))
    {
        cframe_offset *= this->cframe; 
        for(Mesh& mesh : meshes)
        { 
            // uniform buffer
            UniformMeshStruct ums;
            ums.cframe = cframe_offset;
            ums.metalliness = mesh.material.metalliness;
            ums.roughness = mesh.material.roughness;
            ums.base_color = mesh.material.base_color;

            // uniform samplers
            ums.albedo_texture_id = mesh.material.albedo_texture_id;
            ums.metal_roughness_texture_id = mesh.material.metallic_roughness_texture_id;

            if(mesh.material.albedo_texture_id != -1)
            {
                uint32_t tex = mesh.material.albedo_texture_id;
                VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
                descriptors->color_image_pool[tex].destroy();
                descriptors->color_image_pool[tex].create_image(MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, format, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
                descriptors->color_image_pool[tex].fill_memory(MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, 4, mesh.material.albedo_pixels.data());
                descriptors->color_image_pool[tex].create_image_view(format, VK_IMAGE_ASPECT_COLOR_BIT);
            }
            
            if(mesh.material.metallic_roughness_texture_id != -1)
            {
                uint32_t tex = mesh.material.metallic_roughness_texture_id;
                VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
                descriptors->material_image_pool[tex].destroy();
                descriptors->material_image_pool[tex].create_image(MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, format, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
                descriptors->material_image_pool[tex].fill_memory(MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, 4, mesh.material.metallic_roughness_pixels.data());
                descriptors->material_image_pool[tex].create_image_view(format, VK_IMAGE_ASPECT_COLOR_BIT);
            }

            descriptors->dynamic_uniform_buffer.fill_memory(&ums, sizeof(UniformMeshStruct), DYNAMIC_DESCRIPTOR_SIZE * mesh.id );
        }

        for(Node& node : children) node.update_dynamic_buffer(descriptors, cframe_offset);
    }

    // debug ----------------------------------------------------
    void gap(int n){for(int i=0;i<n;i++)msg::print("  ");}
    void iterate(int depth = 0){
        gap(depth); msg::highlight(this->name);
        
        for(Mesh& mesh : meshes){ 
            gap(depth+1); 
            msg::printl(mesh.name," | ", mesh.id, " | ", mesh.material.albedo_texture_id, " | ", mesh.material.metallic_roughness_texture_id); 
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
        msg::success("model buffers created");
    }

public:
    uint32_t total_indices_size = 0;
    uint32_t total_vertices_size = 0;
    
    std::vector<Node> nodes;
    std::vector<MeshDrawInfo> infos;

    // 2
    void prepare_model(Instance* instance, Descriptors* descriptors)
    {
        create_buffers(instance);
        for(Node& node : nodes) node.update_dynamic_buffer(descriptors);
        for(Node& node : nodes) node.get_draw_info(infos);
        for(Node& node : nodes) node.iterate();

        // prepare dynamic buffer
        // create mesh materials
    }

    // 3
    void draw(VkCommandBuffer* cmd, VkPipelineLayout *pipeline_layout, Descriptors *descriptors)
    {
        for(MeshDrawInfo& info : infos){
            std::array<uint32_t, 1> dbo = { DYNAMIC_DESCRIPTOR_SIZE * info.id }; // dynamic buffer offset;
            VkDeviceSize vertex_offset = 0;

            vkCmdBindVertexBuffers(*cmd, 0, 1, &vertices.buffer, &vertex_offset);
            vkCmdBindIndexBuffer(*cmd, indices.buffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdBindDescriptorSets(*cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline_layout, 0, 1, &descriptors->descriptor_sets, 1, dbo.data());
            vkCmdDrawIndexed(*cmd, info.index_count, 1, info.index_offset, info.vertex_offset, 0);
        }
    }

    // 4
    void destroy(){
        vertices.destroy();
        indices.destroy();

    }

    //------------------------------------
    /// combined mesh region

    Region get_region()
    {
        Region r = infos[0].region;
        for(MeshDrawInfo& info : infos)
        {
            //msg::success(info.region.max, info.region.min);
            r.max.x = r.max.x > info.region.max.x? r.max.x : info.region.max.x;
            r.max.y = r.max.y > info.region.max.y? r.max.y : info.region.max.y;
            r.max.z = r.max.z > info.region.max.z? r.max.z : info.region.max.z;

            r.min.x = r.min.x < info.region.min.x? r.min.x : info.region.min.x;
            r.min.y = r.min.y < info.region.min.y? r.min.y : info.region.min.y;
            r.min.z = r.min.z < info.region.min.z? r.min.z : info.region.min.z;
        }
        return r;
    }
};

