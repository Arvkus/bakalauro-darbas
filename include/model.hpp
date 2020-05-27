#include "common.hpp"
#include "descriptors.hpp"

struct MeshDrawInfo{
    uint32_t id = 0;
	uint32_t vertex_offset = 0;
	uint32_t index_offset = 0;
	uint32_t index_count = 0;
	Region region;
};

//-------------------------------------------

class Mesh{
public:
    std::string name = "mesh";

    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    Region region;

    struct Pixels{
        std::vector<uint8_t> albedo;
        std::vector<uint8_t> normal;
        std::vector<uint8_t> material;
        std::vector<uint8_t> emission;
    } pixels;

    struct UniformMeshStruct{
        alignas(16) glm::mat4 cframe = glm::mat4(1.0);
        alignas(16) glm::vec3 base_color = glm::vec3(1.0);
        alignas(16) glm::vec3 emission_factor = glm::vec3(1.0);
        alignas(4) float roughness = 1.0;
        alignas(4) float metalliness = 1.0;
        alignas(4) int32_t albedo_id = -1; // -1; means no texture
        alignas(4) int32_t normal_id = -1; // (occlusion, roughness, metalliness)
        alignas(4) int32_t material_id = -1;
        alignas(4) int32_t emission_id = -1;
    } uniform;

    uint32_t id = 0; // mesh id number
    uint32_t ioffset = 0; // linear memory index offset location
    uint32_t voffset = 0; // linear memory vertex offset location
};

//-------------------------------------------

class Node{
public:
    std::string name = "node";
    glm::mat4 cframe = glm::mat4(1.0);
    std::vector<Node> children;
    std::vector<Mesh> meshes;

    void calculate_vertex_TBN(){
        for(Mesh& mesh : meshes){
            for(uint32_t i = 0; i < mesh.indices.size(); i=i+3)
            {
                // triangle indices
                uint32_t i0 = mesh.indices[i+0]; 
                uint32_t i1 = mesh.indices[i+1];
                uint32_t i2 = mesh.indices[i+2];

                // triangle vertices (from indices)
                std::array<glm::vec3, 3> vertices = {
                    mesh.vertices[i0].position,
                    mesh.vertices[i1].position,
                    mesh.vertices[i2].position,
                };

                std::array<glm::vec2, 3> texcoords = {
                    mesh.vertices[i0].texcoord,
                    mesh.vertices[i1].texcoord,
                    mesh.vertices[i2].texcoord,
                };

                glm::vec3 pos1 = vertices[1] - vertices[0];
                glm::vec3 pos2 = vertices[2] - vertices[0];

                glm::vec2 uv1 = texcoords[1] - texcoords[0];
                glm::vec2 uv2 = texcoords[2] - texcoords[0];

                // get tangent, bitangent based on formula
                float r = 1 / (uv1.x * uv2.y - uv1.y * uv2.x);
                glm::vec3 tangent = (pos1 * uv2.y - pos2 * uv1.y)*r;
                glm::vec3 bitangent = (pos2 * uv1.x - pos1 * uv2.x)*r;

                // normalize
                mesh.vertices[i0].tangent = glm::normalize(tangent);
                mesh.vertices[i1].tangent = glm::normalize(tangent);
                mesh.vertices[i2].tangent = glm::normalize(tangent);

                mesh.vertices[i0].bitangent = glm::normalize(bitangent);
                mesh.vertices[i1].bitangent = glm::normalize(bitangent);
                mesh.vertices[i2].bitangent = glm::normalize(bitangent);
            }
        }
        for(Node& node : children) node.calculate_vertex_TBN();
    }

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

    void update_dynamic_buffer(Instance* instance, Descriptors *descriptors, glm::mat4 cframe_offset = glm::mat4(1.0))
    {
        cframe_offset *= this->cframe; 
        for(Mesh& mesh : meshes)
        {   
            // image buffers
            if(mesh.uniform.albedo_id != -1)
            {
                uint32_t tex = mesh.uniform.albedo_id;
                descriptors->albedo.fill_memory(MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, 4, mesh.pixels.albedo.data(), tex);
                vkDestroyImageView(instance->device, descriptors->albedo_image_views[tex], nullptr);
                descriptors->albedo_image_views[tex] = descriptors->albedo.return_image_view(tex);
            }

            if(mesh.uniform.normal_id != -1)
            {
                uint32_t tex = mesh.uniform.normal_id;
                descriptors->normal.fill_memory(MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, 4, mesh.pixels.normal.data(), tex);
                vkDestroyImageView(instance->device, descriptors->normal_image_views[tex], nullptr);
                descriptors->normal_image_views[tex] = descriptors->normal.return_image_view(tex);
            }
            
            if(mesh.uniform.material_id != -1)
            {
                uint32_t tex = mesh.uniform.material_id;
                descriptors->material.fill_memory(MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, 4, mesh.pixels.material.data(), tex);
                vkDestroyImageView(instance->device, descriptors->material_image_views[tex], nullptr);
                descriptors->material_image_views[tex] = descriptors->material.return_image_view(tex);
            }

            if(mesh.uniform.emission_id != -1)
            {
                uint32_t tex = mesh.uniform.material_id;
                descriptors->emission.fill_memory(MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, 4, mesh.pixels.emission.data(), tex);
                vkDestroyImageView(instance->device, descriptors->emission_image_views[tex], nullptr);
                descriptors->emission_image_views[tex] = descriptors->emission.return_image_view(tex);
            }

            // uniform buffer
            mesh.uniform.cframe = cframe_offset;
            descriptors->dynamic_uniform_buffer.fill_memory(&mesh.uniform, sizeof(mesh.uniform), DYNAMIC_DESCRIPTOR_SIZE * mesh.id );
        }

        for(Node& node : children) node.update_dynamic_buffer(instance, descriptors, cframe_offset);
    }
};

class Model{
private:
    Buffer indices;
    Buffer vertices;

    // 1
    void create_buffers(Instance* instance)
    {
        VkDeviceSize size;

        size = sizeof(uint32_t) * this->total_indices_size;
        if(size == 0) throw std::runtime_error("model buffer is empty");
        indices.init(instance);
        indices.create_buffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        size = sizeof(Vertex) * this->total_vertices_size;
        if(size == 0) throw std::runtime_error("model buffer is empty");
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
        for(Node& node : nodes) node.calculate_vertex_TBN();
        create_buffers(instance);
        for(Node& node : nodes) node.update_dynamic_buffer(instance, descriptors);
        for(Node& node : nodes) node.get_draw_info(infos);
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

