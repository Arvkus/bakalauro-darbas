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
    uint32_t index_offset = 0;
    uint32_t vertex_offset = 0;

    void draw(VkCommandBuffer command)
    {
        /*
        vkCmdBindVertexBuffers(command, 0, 1, &model.vertex_buffer.buffer, offsets);
        vkCmdBindIndexBuffer(command, model.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT16);

        vkCmdBindDescriptorSets(
            command, 
            VK_PIPELINE_BIND_POINT_GRAPHICS, 
            pipeline.pipeline_layout, 0, 1, 
            &descriptors.descriptor_sets[i], 
            0, nullptr
        );

        //vkCmdDraw(command, (uint32_t)model.vertices.size(), 1, 0, 0);
        vkCmdDrawIndexed(command, (uint32_t)model.meshes[0].indices.size(), 1, 0, 0, 0);
        */

    }

private:

};


class Model{
public:
    Buffer vertex_buffer;
    Buffer index_buffer;

    std::string name;
    std::vector<Mesh> meshes; 

    uint32_t v_count = 0;
    uint32_t i_count = 0;

    void gap(int n){for(int i=0;i<n;i++)msg::print("  ");}

    void create_buffers(Instance *instance)
    {
        v_count = count_vertices(meshes);
        i_count = count_indices(meshes);

        calc_vertex_offset(meshes);

        msg::error(v_count, " ", i_count);
        //VkDeviceSize isize = get_index_size(meshes);

        VkDeviceSize size;
        size = sizeof(Vertex) * meshes[0].vertices.size();
        vertex_buffer.init(instance);
        vertex_buffer.create_buffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vertex_buffer.fill_memory(meshes[0].vertices.data(), size);

        size = sizeof(uint16_t) * meshes[0].indices.size();
        index_buffer.init(instance);
        index_buffer.create_buffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        index_buffer.fill_memory(meshes[0].indices.data(), size);
        

        /*
        vertex_buffer.init(instance);
        vertex_buffer.create_buffer(vsize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        fill_vertex_buffer(meshes);

        index_buffer.init(instance);
        index_buffer.create_buffer(isize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        fill_index_buffer(meshes);
        */


        printf("Vertex and index buffers created \n");
    }

    void destroy(){
        vertex_buffer.destroy();
        index_buffer.destroy();
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

    //------------------------------------------------------

    uint32_t calc_vertex_offset(std::vector<Mesh>& meshes, uint32_t offset = 0){
        for(Mesh& mesh : meshes){
            msg::success(mesh.name, " - ", offset );
            mesh.vertex_offset = offset;
            offset += mesh.vertices.size();
            offset += calc_vertex_offset(mesh.children, mesh.vertices.size());
        }
        return offset;
    }

    uint32_t calc_index_offset(std::vector<Mesh>& meshes){
        uint32_t size = 0;
        for(Mesh& mesh : meshes){
            size += (uint32_t)mesh.vertices.size();
            size += calc_index_offset(mesh.children);
        }
        return size;
    }


    /*
    uint32_t get_vertex_size(std::vector<Mesh>& meshes, int depth = 0, int offset = 0){
        uint32_t size = 0;
        for(Mesh& mesh : meshes){
            mesh.vertex_mem_offset = offset + size;
            //gap(depth); msg::success(mesh.name, " ", offset + size);

            size += (uint32_t)mesh.vertices.size();

            uint32_t children_size = get_vertex_size(mesh.children, depth+1, offset+size);
            size += children_size;
        }
        return size;
    }

    void fill_vertex_buffer(const std::vector<Mesh>& meshes){
        for(const Mesh& mesh : meshes){
            vertex_buffer.fill_memory(mesh.vertices.data(), sizeof(Vertex) * mesh.vertices.size(), mesh.vertex_mem_offset );
            fill_vertex_buffer(mesh.children);
        }
    }

    //-----------------

    uint32_t get_index_size(std::vector<Mesh>& meshes, int depth = 0, int offset = 0){
        uint32_t size = 0;
        for(Mesh& mesh : meshes){
            
            mesh.index_mem_offset = offset + size;
            size += (uint32_t)mesh.indices.size();

            // gap(depth); msg::success(mesh.name, " ", size);

            uint32_t children_size = get_index_size(mesh.children, depth+1, offset + size);
            size += children_size;
        }
        return size;
    }

    void fill_index_buffer(const std::vector<Mesh>& meshes){
        for(const Mesh& mesh : meshes){
            index_buffer.fill_memory(mesh.indices.data(), sizeof(uint32_t) * mesh.indices.size(), mesh.index_mem_offset );
            fill_index_buffer(mesh.children);
        }
    }
    */



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
