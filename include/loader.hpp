#pragma once
#include "common.hpp"
using json = nlohmann::json;

float test1 = 1.0;
float test2 = 0.0;
// https://wiki.fileformat.com/3d/glb/
// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#properties-reference
//------------------------------------------------------------------------------

class Loader{
private:
    json content; // json chunk
    std::vector<char> buffer; // binary chunk

    struct MemoryInfo{ // get buffer binary data offset/size/stride 
        uint32_t offset;
        uint32_t stride;
        uint32_t length;
    };

    /// Check if json has value
    bool is(const json& node, const std::string& value){ return node.find(value) != node.end(); }
    /// Print space 'n' amount of times
    void gap(int n){for(int i=0;i<n;i++)msg::print("  ");}

    //----------------------------------------------------
    /// get primitive's memory location/size
    
    MemoryInfo get_memory_info(const uint32_t accessor_id)
    {
        json accessor = content["accessors"][accessor_id];

        std::string type = accessor["type"];
        uint32_t buffer_view_id = accessor["bufferView"];
        uint32_t accessor_offset = is(accessor,"byteOffset")? accessor["byteOffset"] : 0;

        json buffer_view = content["bufferViews"][buffer_view_id]; // byteOffset

        uint32_t buffer_offset = is(buffer_view, "byteOffset")? buffer_view["byteOffset"] : 0;
        //uint32_t buffer_length = is(buffer_view, "byteLength")? buffer_view["byteLength"] : 0;
        uint32_t stride; // validate if data types are supported by this parser

        //-----------------------------------------------------
        //if(type == "VEC4"){ stride = 4 * 4; msg::error("vec4");} else  // float = 4 bytes, vec3 = 3 floats
        if(type == "VEC3") stride = 4 * 3; else  // float = 4 bytes, vec3 = 3 floats
        if(type == "VEC2") stride = 4 * 2; else
        if(type == "SCALAR"){ 
            if(accessor["componentType"] == 5123) stride = 2; else // unsignet short (2 bytes)
            if(accessor["componentType"] == 5125) stride = 4; else // unsigned int (4 bytes)
            throw std::runtime_error("nuknown accessor component type");
        }else throw std::runtime_error("unknown accessor type");
        //----------------------------------------------------
        MemoryInfo memory = {};
        memory.offset = buffer_offset + accessor_offset;
        memory.length = (uint32_t)accessor["count"] * stride; // byte length
        memory.stride = stride;

        return memory;
    }

    //----------------------------------------------------
    /// get all primitive data and construct vertices

    std::vector<Vertex> create_vertices(const json& primitive)
    {
        MemoryInfo memory;
        uint32_t accessor_id;

        std::vector<uint32_t> indices;
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texcoords;
        
        //------------------------------------------------
         
        accessor_id = primitive["attributes"]["POSITION"];
        memory = get_memory_info(accessor_id);
        positions.resize(memory.length / memory.stride);

        for(uint32_t i = 0; i < positions.size(); i++){
            std::memcpy(&positions[i], buffer.data() + memory.offset + i*memory.stride, memory.stride);
        }
        //------------------------------------------------
        accessor_id = primitive["attributes"]["NORMAL"];
        memory = get_memory_info(accessor_id);
        normals.resize(memory.length / memory.stride);

        for(uint32_t i = 0; i < normals.size(); i++){
            std::memcpy(&normals[i], buffer.data() + memory.offset + i*memory.stride, memory.stride);
        }
        //------------------------------------------------
        
        // texcoord is optional
        if(is(primitive["attributes"],"TEXCOORD_0")){
            accessor_id = primitive["attributes"]["TEXCOORD_0"];
            memory = get_memory_info(accessor_id);
            texcoords.resize(memory.length / memory.stride);

            for(uint32_t i = 0; i < texcoords.size(); i++){
                std::memcpy(&texcoords[i], buffer.data() + memory.offset + i*memory.stride, memory.stride);
            }
        }else{
            texcoords.resize(positions.size());
        }
        //------------------------------------------------
        // mesh can be without indices
        if(is(primitive,"indices")){
            accessor_id = primitive["indices"];
            memory = get_memory_info(accessor_id);
            indices.resize(memory.length / memory.stride);

            for(uint32_t i = 0; i < indices.size(); i++){
                std::memcpy(&indices[i], buffer.data() + memory.offset + i*memory.stride, memory.stride);
            }
        }else{
            throw std::runtime_error("Mesh is without indices, indices are required");
        }
        //------------------------------------------------
        // construct vertices
        std::vector<Vertex> vertices(positions.size());
        if(positions.size() == normals.size() && normals.size() == texcoords.size()){
            for(uint32_t i = 0; i < positions.size(); i++){
                vertices.push_back( Vertex(positions[i], normals[i], texcoords[i]));
            }
        }else{
            throw std::runtime_error("Vertex primitive data length is not equal.");
        }

        return vertices;
    }
    

    //----------------------------------------------------
    /// get primitive's texture pixel data

    std::vector<char> get_texture_pixels(uint32_t texture_index)
    {
        uint32_t source_index = content["textures"][texture_index]["source"];
        json image = content["images"][source_index];
        
        json buffer_view = content["bufferViews"][(uint32_t)image["bufferView"]];
        uint32_t byte_length = buffer_view["byteLength"];
        uint32_t byte_offset = is(buffer_view,"byteOffset")? buffer_view["byteOffset"] : 0;

        int width = 0, height = 0, channel = 0;
        stbi_uc* pixels = stbi_load_from_memory( 
            reinterpret_cast<const stbi_uc*>(buffer.data()) + byte_offset, 
            byte_length,
            &width, &height, &channel,
            STBI_rgb_alpha
        );

        // resize and write to pixel buffer
        std::vector<char> pixel_buffer(MAX_IMAGE_SIZE * MAX_IMAGE_SIZE * 4);
        stbir_resize_uint8(pixels, width , height , 0, (unsigned char*)pixel_buffer.data(), MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, 0, 4);
        stbi_image_free(pixels);
        return pixel_buffer;
    } 
    
    //----------------------------------------------------
    /// Scene is made out of `nodes`, each node can have more nodes as children

    std::vector<Mesh> build_meshes(const json& nodes, int depth = 0)
    {
        std::vector<Mesh> model_meshes;

        // node
        for(uint32_t node_id: nodes)
        {
            json node = content["nodes"][node_id];
            gap(depth); msg::highlight(node["name"] == "null"? node["name"] : "null");

            // mesh
            if(is(node,"mesh"))
            {
                json mesh = content["meshes"][ (uint32_t)node["mesh"] ];
                gap(depth+1); msg::success(mesh["name"]);

                // primitive
                for(json primitive : mesh["primitives"])
                {   
                    gap(depth+2); msg::printl("primitive");

                    // vertices
                    std::vector<Vertex> vertices = create_vertices(primitive);

                    // material
                    if(is(primitive, "material"))
                    {
                        uint32_t material_id = primitive["material"];
                        json material = content["materials"][material_id];
                        gap(depth+2); msg::printl(material["name"]);


                        if(is(material,"pbrMetallicRoughness")){
                            json pbr = material["pbrMetallicRoughness"];

                            // pbr textures
                            if(is(pbr,"baseColorTexture")){
                                uint32_t texture_index = pbr["baseColorTexture"]["index"];
                                std::vector<char> pixels = get_texture_pixels(texture_index);
                            }

                            if(is(pbr,"metallicRoughnessTexture")){
                                uint32_t texture_index = pbr["metallicRoughnessTexture"]["index"];
                                std::vector<char> pixels = get_texture_pixels(texture_index);
                            }

                            // pbr vertices
                            if(is(pbr,"baseColorFactor")) {};
                            if(is(pbr,"metallicFactor")) {};
                            if(is(pbr,"roughnessrFactor")) {};
                        } // pbr

                    } // material

                } // primitives
                
            } // mesh

            if(is(node,"children")){
                build_meshes(node["children"], depth+1);
            }
            
        }
        
        return model_meshes;
    }

    //----------------------------------------------------
public:
    Model load_glb(const char* path){ 
        try{
            // 0x46546C67 - file magic
            // 0x4E4F534A - JSON chunk type
            // 0x004E4942 - Bin chunk type 

            uint64_t start_time = timestamp_milli();
            std::ifstream file(path, std::ifstream::binary);
            uint32_t chunk_length, chunk_type;
            

            //-------------------------------
            // check if file is valid

            if(!file.is_open()){
                throw std::runtime_error(std::string("Failed to open: ") + path);
            }else{
                uint32_t magic;
                uint32_t version;
                uint32_t length;

                file.read((char*)&magic, 4);
                file.read((char*)&version, 4);
                file.read((char*)&length, 4);

                if(0x46546C67 == magic){
                    msg::print(path, " | 'glb", version, "' file format | ", length, " bytes (", (float)length/1024/1024, " MB)\n");
                }else{
                    throw std::runtime_error(std::string("file is not .glb or is corrupted: ") + path);
                }
            }

            //-------------------------------
            // read JSON

            file.read((char*)&chunk_length, 4);
            file.read((char*)&chunk_type, 4);

            if(chunk_type != 0x4E4F534A) throw std::runtime_error(std::string("file is corrupted: ") + path);

            buffer.resize(chunk_length);
            file.read(buffer.data(), chunk_length);

            try{
                content = content.parse(buffer);
            }catch(json::exception& e){
                throw e;
            }

            // output to file for debug
            std::ofstream out("debug.json");
            out << content.dump(4); out.close();

            
            //-------------------------------
            // read binary buffers

            file.read((char*)&chunk_length, 4);
            file.read((char*)&chunk_type, 4);

            if(chunk_type != 0x004E4942) throw std::runtime_error(std::string("file is corrupted: ") + path);

            buffer.resize(chunk_length);
            file.read(buffer.data(), chunk_length);
            file.close();

            Model model;
            model.meshes = build_meshes(content["scenes"][0]["nodes"]);
            
            msg::print("Time to create model: ", (float)(timestamp_milli() - start_time)/1000, "\n");
            return model;
        }catch(const json::exception& e){
            msg::warn(std::string("Loader: ") + e.what());
            return Model();
        }catch(const std::exception& e){
            msg::warn(std::string("Loader: ") + e.what());
            return Model();
        };
    }
};
