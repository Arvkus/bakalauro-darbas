#pragma once
#include "common.hpp"
using json = nlohmann::json;

// https://wiki.fileformat.com/3d/glb/
// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#properties-reference
//------------------------------------------------------------------------------

class Loader{
private:
    json content; // json chunk
    std::vector<char> buffer; // binary chunk

    /// Check if json has value
    bool is(const json& node, const std::string& value){ return node.find(value) != node.end(); }
    /// Print space 'n' amount of times
    void gap(int n){for(int i=0;i<n;i++)msg::print("  ");}

    //----------------------------------------------------
    /// get primitive's all buffer bytes
    
    uint32_t get_memory_offset(const json& accessor)
    {
        std::string type = accessor["type"];
        uint32_t buffer_view_id = accessor["bufferView"];
        uint32_t accessor_offset = is(accessor,"byteOffset")? accessor["byteOffset"] : 0;

        json buffer_view = content["bufferViews"][buffer_view_id]; // byteOffset
        uint32_t buffer_offset = is(buffer_view, "byteOffset")? buffer_view["byteOffset"] : 0;

        //-----------------------------------------------------
        uint32_t stride; // validate if data types are supported by this parser
        if(type == "VEC3") stride = 4 * 3; else  // float = 4 bytes, vec3 = 3 floats
        if(type == "VEC2") stride = 4 * 2; else
        if(type == "SCALAR"){ 
            if(accessor["componentType"] == 5123) stride = 2; else // unsignet short (2 bytes)
            //if(accessor["componentType"] == 5125) stride = 4; else // unsigned int (4 bytes)
            throw std::runtime_error("nknown accessor component type");
        }else throw std::runtime_error("unknown accessor type");
        //----------------------------------------------------
        return buffer_offset + accessor_offset;
    }
    

    //----------------------------------------------------

    void validate_accessor(uint32_t attribute_id)
    {
        json attribute = content["accessors"][attribute_id];
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

            // get mesh metadata
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

            gap(depth);
            msg::highlight(model_mesh.name);

            // get mesh primitives
            // https://community.khronos.org/t/questions-about-multiple-primitives-per-mesh-in-gltf-file/104013/2
            // PATIKRINTI INDEKSUS ??
            
            json mesh =  content["meshes"][(int)node["mesh"]];
            for(json primitive : mesh["primitives"])
            {
                Mesh primitive_mesh;

                if(!is(primitive,"indices")) std::runtime_error("mesh doesn't have indices");
                if(!is(primitive["attributes"],"NORMAL")) std::runtime_error("mesh doesn't have normals");
                if(!is(primitive["attributes"],"POSITION")) std::runtime_error("mesh doesn't have positions");
                if(!is(primitive["attributes"],"TEXCOORD_0")) std::runtime_error("mesh doesn't have texcoords");

                uint32_t accessor_id; 
                uint32_t offset;
                json accessor;

                //----------------------------------------------------------------
                accessor_id = primitive["attributes"]["POSITION"];
                accessor = content["accessors"][accessor_id];
                offset = get_memory_offset(accessor);
                std::vector<glm::vec3> positions(accessor["count"]);
                
                for(uint32_t i = 0; i < positions.size(); i++){
                    std::memcpy(&positions[i], buffer.data() + offset + i*4*3, 4*3);
                }
                //----------------------------------------------------------------
                accessor_id = primitive["attributes"]["NORMAL"];
                accessor = content["accessors"][accessor_id];
                offset = get_memory_offset(accessor);
                std::vector<glm::vec3> normals(accessor["count"]);

                for(uint32_t i = 0; i < normals.size(); i++){
                    std::memcpy(&normals[i], buffer.data() + offset + i*4*3, 4*3);
                }

                //----------------------------------------------------------------
                accessor_id = primitive["attributes"]["TEXCOORD_0"];
                accessor = content["accessors"][accessor_id];
                offset = get_memory_offset(accessor);
                std::vector<glm::vec3> texcoords(accessor["count"]);

                for(uint32_t i = 0; i < texcoords.size(); i++){
                    std::memcpy(&texcoords[i], buffer.data() + offset + i*4*2, 4*2);
                }
                //----------------------------------------------------------------
                accessor_id = primitive["indices"];
                accessor = content["accessors"][accessor_id];
                offset = get_memory_offset(accessor);
                primitive_mesh.indices.resize(accessor["count"]);
                
                for(uint32_t i = 0; i < primitive_mesh.indices.size(); i++){
                    std::memcpy(&primitive_mesh.indices[i], buffer.data() + offset + i*2, 2);
                    //msg::print(model_mesh.indices[i], " ");
                }
                
                //----------------------------------------------------------------
                // construct vertices
                if(positions.size() == normals.size() && normals.size() == texcoords.size()){
                    for(uint32_t i = 0; i < positions.size(); i++){
                        primitive_mesh.vertices.push_back( Vertex(positions[i], normals[i], texcoords[i]));
                    }
                }else{
                    throw std::runtime_error("Vertex primitive data length is not equal.");
                }
                //----------------------------------------------------------------

                primitive_mesh.translation = model_mesh.translation;
                primitive_mesh.scale = model_mesh.scale;
                primitive_mesh.rotation = model_mesh.rotation;

                // primitive mesh
                model_meshes.push_back(primitive_mesh);
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
