#include "common.hpp"

class Mesh{
public:
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;

    glm::vec3 scale = glm::vec3(1);
    glm::vec3 translation = glm::vec3(0);
    glm::quat rotation = glm::quat(1,0,0,0);

    std::vector<Mesh> children;

private:

};


class Model{
public:
    Buffer vertex_buffer;
    Buffer index_buffer;

    std::string name;
    std::vector<Mesh> meshes; 

    void create_buffers(Instance *instance)
    {
        VkDeviceSize size;

        size = sizeof(Vertex) * meshes[0].vertices.size();
        vertex_buffer.init(instance);
        vertex_buffer.create_buffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vertex_buffer.fill_memory(meshes[0].vertices.data(), size);

        size = sizeof(uint16_t) * meshes[0].indices.size();
        index_buffer.init(instance);
        index_buffer.create_buffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        index_buffer.fill_memory(meshes[0].indices.data(), size);

        printf("Vertex and index buffers created \n");
    }

    void destroy(){
        vertex_buffer.destroy();
        index_buffer.destroy();
    }

private:
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
