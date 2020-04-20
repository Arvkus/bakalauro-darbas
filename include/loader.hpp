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
    uint32_t primitive_index = 0;

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
    /// get primitive's all buffer bytes
    
    MemoryInfo get_memory_offset(const json& accessor)
    {
        std::string type = accessor["type"];
        uint32_t buffer_view_id = accessor["bufferView"];
        uint32_t accessor_offset = is(accessor,"byteOffset")? accessor["byteOffset"] : 0;

        json buffer_view = content["bufferViews"][buffer_view_id]; // byteOffset

        uint32_t buffer_offset = is(buffer_view, "byteOffset")? buffer_view["byteOffset"] : 0;
        uint32_t buffer_length = is(buffer_view, "byteLength")? buffer_view["byteLength"] : 0;
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
        memory.length = buffer_length;
        memory.stride = stride;
        return memory;
    }
    
    //----------------------------------------------------
    /// Scene is made out of `nodes`, each node can have more nodes as children
    std::vector<Mesh> build_meshes(const json& nodes, int depth = 0)
    {
        
        std::vector<Mesh> model_meshes;

        for(uint32_t node_id: nodes)
        {
            json node = content["nodes"][node_id];
            if(!is(node,"mesh")) continue; // node can be without mesh, skip nodes without mesh

            Mesh model_mesh = Mesh();

            // get mesh additional data !!!! node data not mesh
            model_mesh.name = is(node,"name")? node["name"] : "null";

            model_mesh.translation = is(node, "translation")? glm::vec3( 
                node["translation"][0],
                node["translation"][1],
                node["translation"][2]
            ) : glm::vec3(0);

            model_mesh.scale = is(node, "scale")? glm::vec3( 
                node["scale"][0],
                node["scale"][1],
                node["scale"][2]
            ) : glm::vec3(1);

            model_mesh.rotation = is(node, "rotation")? glm::quat( 
                node["rotation"][3],
                node["rotation"][0],
                node["rotation"][1],
                node["rotation"][2]
            ) : glm::quat(1,0,0,0);

            gap(depth); // debug output
            msg::highlight(model_mesh.name);

            // get mesh primitives
            json mesh = content["meshes"][(uint32_t)node["mesh"]];
            for(json primitive : mesh["primitives"])
            {
                Primitive po; // primitive object
                po.primitive_index = primitive_index;

                gap(depth+1);
                msg::printl("primitive ", primitive_index);

                if(!is(primitive,"indices")) throw std::runtime_error("mesh doesn't have indices");
                if(!is(primitive["attributes"],"NORMAL")) throw std::runtime_error("mesh doesn't have normals");
                if(!is(primitive["attributes"],"POSITION")) throw std::runtime_error("mesh doesn't have positions");

                uint32_t accessor_id; 
                MemoryInfo memory;
                json accessor;

                std::vector<glm::vec2> texcoords;
                std::vector<glm::vec3> positions;
                std::vector<glm::vec3> normals;
         
                //----------------------------------------------------------------
                accessor_id = primitive["attributes"]["POSITION"];
                accessor = content["accessors"][accessor_id];
                memory = get_memory_offset(accessor);
                positions.resize(accessor["count"]);
                
                for(uint32_t i = 0; i < positions.size(); i++){
                    std::memcpy(&positions[i], buffer.data() + memory.offset + i*memory.stride, memory.stride);
                }
                //----------------------------------------------------------------
                accessor_id = primitive["attributes"]["NORMAL"];
                accessor = content["accessors"][accessor_id];
                memory = get_memory_offset(accessor);
                normals.resize(accessor["count"]);

                for(uint32_t i = 0; i < normals.size(); i++){
                    std::memcpy(&normals[i], buffer.data() + memory.offset + i*memory.stride, memory.stride);
                }

                //----------------------------------------------------------------
                if(is(primitive["attributes"],"TEXCOORD_0")) // check if texcoords exist
                {
                    accessor_id = primitive["attributes"]["TEXCOORD_0"];
                    accessor = content["accessors"][accessor_id];
                    memory = get_memory_offset(accessor);
                    texcoords.resize(accessor["count"]);

                    for(uint32_t i = 0; i < texcoords.size(); i++){
                        std::memcpy(&texcoords[i], buffer.data() + memory.offset + i*memory.stride, memory.stride);
                    }
                }else
                {
                    texcoords.resize(positions.size()); // resize base on positions
                }
                //----------------------------------------------------------------
                accessor_id = primitive["indices"];
                accessor = content["accessors"][accessor_id];
                memory = get_memory_offset(accessor);
                po.indices.resize(accessor["count"]);
                for(uint32_t i = 0; i < po.indices.size(); i++){
                    std::memcpy(&po.indices[i], buffer.data() + memory.offset + i*memory.stride, memory.stride);
                }
                
                
                //----------------------------------------------------------------
                // construct vertices
                if(positions.size() == normals.size() && normals.size() == texcoords.size()){
                    for(uint32_t i = 0; i < positions.size(); i++){
                        po.vertices.push_back( Vertex(positions[i], normals[i], texcoords[i]));
                    }
                }else{
                    throw std::runtime_error("Vertex primitive data length is not equal.");
                }
                //----------------------------------------------------------------
                // materials
                if(is(primitive,"material")){
                    gap(depth+1);
                    msg::success("has meterial");
                    po.material.roughness = 1.0;
                }else{
                    po.material.roughness = 0.0;
                }
                 //----------------------------------------------------------------
                // primitive mesh
                model_mesh.primitives.push_back(po);
                primitive_index++;
            }

            // for(const Vertex& vertex : model_mesh.vertices)print(vertex.position, vertex.normal, vertex.texture, "\n");

            // loop through all children recursively
            if(is(node,"children")) model_mesh.children = build_meshes(node["children"], depth+1);
            model_meshes.push_back(model_mesh);
            
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
            primitive_index = 0;
            model.name = path;

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
