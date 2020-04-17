#include "common.hpp"
#include "descriptors.hpp"
class Mesh{
public:
    std::string name;
    std::vector<Vertex> vertices = {};
    std::vector<uint32_t> indices = {};

    glm::vec3 scale = glm::vec3(1);
    glm::vec3 translation = glm::vec3(0);
    glm::quat rotation = glm::quat(1,0,0,0);
    glm::mat4 matrix;

    std::vector<Mesh> children;

    bool is_buffers_ready = false;
    Buffer vertex_buffer;
    Buffer index_buffer;

    glm::mat4 construct_matrix(){
        // translate, rotate, scale
        glm::mat4 matrix = glm::mat4(1.0);
        matrix = glm::translate(matrix, translation); // translate
        matrix = matrix * glm::toMat4(rotation); // rotate
        matrix = glm::scale(matrix, scale); // scale
        this->matrix = matrix;
        return matrix;
    }

    void create_buffers(Instance *instance, glm::mat4 offset)
    {
        
        offset *= construct_matrix();

        if(!vertices.size() == 0 && !indices.size() == 0) {
            is_buffers_ready = true;
            VkDeviceSize size;
            
            for(Vertex& vertex : vertices){
                vertex.position = offset * glm::vec4(vertex.position, 1.0);
            }

            size = sizeof(Vertex) * this->vertices.size();
            vertex_buffer.init(instance);
            vertex_buffer.create_buffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            vertex_buffer.fill_memory(this->vertices.data(), size);

            size = sizeof(uint32_t) * this->indices.size();
            index_buffer.init(instance);
            index_buffer.create_buffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            index_buffer.fill_memory(this->indices.data(), size);
        }

        for(Mesh& mesh : children){
            mesh.create_buffers(instance, offset);
        }
        
    }
        
    void destroy(){
        vertex_buffer.destroy();
        index_buffer.destroy();
    }

    void draw(VkCommandBuffer* cmd, VkPipelineLayout *pipeline_layout, Descriptors *descriptors)
    {
        // bindSet(layout, 0, set) for textures
        if(is_buffers_ready){
            VkDeviceSize offsets[1] = {0};
            vkCmdBindVertexBuffers(*cmd, 0, 1, &vertex_buffer.buffer, offsets);
            vkCmdBindIndexBuffer(*cmd, index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdBindDescriptorSets(*cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline_layout, 0, 1, 
            &descriptors->descriptor_sets[0], 0, nullptr);
            vkCmdDrawIndexed(*cmd, (uint32_t)indices.size(), 1, 0, 0, 0);
        }

        for(Mesh& mesh : children){
            mesh.draw(cmd, pipeline_layout, descriptors);
        }
    }

private:

};


class Model{
public:
    std::string name;
    std::vector<Mesh> meshes; 


    void gap(int n){for(int i=0;i<n;i++)msg::print("  ");}

    void create_buffers(Instance *instance)
    {
        uint32_t count = count_vertices(meshes);
        //msg::error(count);

        for(Mesh& mesh: meshes){
            mesh.create_buffers(instance, glm::mat4(1.0));
        }
    }

    void draw(VkCommandBuffer *cmd, VkPipelineLayout *pipeline_layout, Descriptors *descriptors)
    {
        for(Mesh& mesh: meshes){
            mesh.draw(cmd, pipeline_layout, descriptors);
        }
    }

    void destroy(){

    }

private:

    //------------------------------------------------------
    uint32_t count_vertices(std::vector<Mesh>& meshes){
        uint32_t size = 0;
        for(Mesh& mesh : meshes){
            size += (uint32_t)mesh.vertices.size();
            size += count_vertices(mesh.children);
        }
        return size;
    }

    uint32_t count_indices(std::vector<Mesh>& meshes){
        uint32_t size = 0;
        for(Mesh& mesh : meshes){
            size += (uint32_t)mesh.indices.size();
            size += count_indices(mesh.children);
        }
        return size;
    }
};





/*
class Model2{
public:

    std::vector<Vertex> vertices = {
        Vertex(glm::vec3(-0.5,-0.5, 0.0), glm::vec3(1,0,0), glm::vec2(1,0)),
        Vertex(glm::vec3( 0.5,-0.5, 0.0), glm::vec3(0,1,0), glm::vec2(0,0)),
        Vertex(glm::vec3( 0.5, 0.5, 0.0), glm::vec3(0,0,1), glm::vec2(0,1)),
        Vertex(glm::vec3(-0.5, 0.5, 0.0), glm::vec3(1,1,0), glm::vec2(1,1)),

        Vertex(glm::vec3(-0.5,-0.5,-0.5), glm::vec3(1,0,0), glm::vec2(1,0)),
        Vertex(glm::vec3( 0.5,-0.5,-0.5), glm::vec3(0,1,0), glm::vec2(0,0)),
        Vertex(glm::vec3( 0.5, 0.5,-0.5), glm::vec3(0,0,1), glm::vec2(0,1)),
        Vertex(glm::vec3(-0.5, 0.5,-0.5), glm::vec3(1,1,0), glm::vec2(1,1)),
    };

    std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
    };
};
*/
