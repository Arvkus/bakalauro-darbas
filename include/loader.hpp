#pragma once
#include "common.hpp"

class Loader{
    using json = nlohmann::json;
private:

    enum TYPE { GLB, GLTF };

    json content; // json chunk
    std::vector<char> buffer; // binary chunk
    std::string name; // for glTF
    std::string folder;
    TYPE type;

    struct MemoryInfo{ // get buffer binary data offset/size/stride 
        uint32_t offset;
        uint32_t stride;
        uint32_t length;
    };

    glm::mat4 construct_cframe(glm::vec3 translation, glm::quat rotation, glm::vec3 scale){
        // translate, rotate, scale
        glm::mat4 matrix = glm::mat4(1.0);
        matrix = glm::translate(matrix, translation); // translate
        matrix = matrix * glm::toMat4(rotation); // rotate
        matrix = glm::scale(matrix, scale); // scale
        return matrix;
    }

    struct Counter{
        uint32_t mesh = 0;
        uint32_t indices = 0;
        uint32_t vertices = 0;
        uint32_t albedo_texture = 0;
        uint32_t normal_texture = 0;
        uint32_t material_texture = 0;
        uint32_t emission_texture = 0;

        void reset(){
            this->mesh = 0;
            this->indices = 0;
            this->vertices = 0;
            this->albedo_texture = 0;
            this->normal_texture = 0;
            this->material_texture = 0;
            this->emission_texture = 0;
        }
    } counter;

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
    // get min/max
    Region get_region(const json& primitive)
    {
        Region r;
        try{
            uint32_t accessor_id = primitive["attributes"]["POSITION"];
            json accessor = content["accessors"][accessor_id];

            r.min = glm::vec3( accessor["min"][0], accessor["min"][1], accessor["min"][2]);
            r.max = glm::vec3( accessor["max"][0], accessor["max"][1], accessor["max"][2]);
        }catch(const json::exception& e){
            msg::warn(std::string("Failed to get region: ") + e.what());
        }
        return r;
    }

    //----------------------------------------------------
    // get vertices

    std::vector<uint32_t> create_indices(const json& primitive)
    {
        std::vector<uint32_t> indices;

        // mesh can be without indices
        if(is(primitive,"indices")){
            uint32_t accessor_id = primitive["indices"];
            MemoryInfo memory = get_memory_info(accessor_id);
            indices.resize(memory.length / memory.stride);

            for(uint32_t i = 0; i < indices.size(); i++){
                std::memcpy(&indices[i], buffer.data() + memory.offset + i*memory.stride, memory.stride);
            }
        }else{
            throw std::runtime_error("Mesh is without indices, indices are required");
        }

        return indices;
    }

    //----------------------------------------------------
    /// get all primitive data and construct vertices

    std::vector<Vertex> create_vertices(const json& primitive)
    {
        MemoryInfo memory;
        uint32_t accessor_id;

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
        // construct vertices
        std::vector<Vertex> vertices(positions.size());
        if(positions.size() == normals.size() && normals.size() == texcoords.size()){
            for(uint32_t i = 0; i < positions.size(); i++){
                vertices[i] = Vertex(positions[i], normals[i], texcoords[i]);
            }
        }else{
            throw std::runtime_error("Vertex primitive data length is not equal.");
        }

        return vertices;
    }
    

    //----------------------------------------------------
    /// get primitive's texture pixel data

    std::vector<uint8_t> get_texture_pixels(uint32_t texture_index)
    {
        int width = 0, height = 0, channel = 0;
        stbi_uc* pixels;

        uint32_t source_index = content["textures"][texture_index]["source"];
        json image = content["images"][source_index];

        if(this->type == TYPE::GLB)
        {
            json buffer_view = content["bufferViews"][(uint32_t)image["bufferView"]];
            uint32_t byte_length = buffer_view["byteLength"];
            uint32_t byte_offset = is(buffer_view,"byteOffset")? buffer_view["byteOffset"] : 0;

            pixels = stbi_load_from_memory( 
                reinterpret_cast<const stbi_uc*>(buffer.data()) + byte_offset, 
                byte_length,
                &width, &height, &channel,
                STBI_rgb_alpha
            );
        }else if(this->type == TYPE::GLTF)
        {
            std::string uri = image["uri"];
            pixels = stbi_load((this->folder + uri).c_str(), &width, &height, &channel, STBI_rgb_alpha);
        }

        // resize and write to pixel buffer
        std::vector<uint8_t> pixel_buffer(MAX_IMAGE_SIZE * MAX_IMAGE_SIZE * 4);
        stbir_resize_uint8(pixels, width , height , 0, (uint8_t*)pixel_buffer.data(), MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, 0, 4);
        stbi_image_free(pixels);
        return pixel_buffer;
    } 

    //----------------------------------------------------

    void fill_material_data(uint32_t material_id, Mesh& mesh, int depth = 0)
    {
        json material = content["materials"][material_id];
        gap(depth); msg::printl(material["name"]);

        if(is(material,"pbrMetallicRoughness")){
            json pbr = material["pbrMetallicRoughness"];

            // PBR textures
            if(is(pbr,"baseColorTexture")){
                uint32_t texture_index = pbr["baseColorTexture"]["index"];
                mesh.pixels.albedo = get_texture_pixels(texture_index);
                mesh.uniform.albedo_id = counter.albedo_texture++;
            }

            if(is(pbr,"metallicRoughnessTexture")){
                uint32_t texture_index = pbr["metallicRoughnessTexture"]["index"];
                mesh.pixels.material = get_texture_pixels(texture_index);
                mesh.uniform.material_id = counter.material_texture++;
            }

            // PBR factors
            if(is(pbr,"baseColorFactor")) mesh.uniform.base_color = glm::vec3(pbr["baseColorFactor"][0], pbr["baseColorFactor"][1], pbr["baseColorFactor"][2]);
            if(is(pbr,"metallicFactor"))  mesh.uniform.metalliness = pbr["metallicFactor"];
            if(is(pbr,"roughnessFactor")) mesh.uniform.roughness = pbr["roughnessFactor"];
            
        }

        if(is(material,"normalTexture")){
            uint32_t texture_index = material["normalTexture"]["index"];
            mesh.pixels.normal = get_texture_pixels(texture_index);
            mesh.uniform.normal_id = counter.normal_texture++;
        }

        if(is(material,"emissiveTexture")){
            uint32_t texture_index = material["emissiveTexture"]["index"];
            mesh.pixels.emission = get_texture_pixels(texture_index);
            mesh.uniform.emission_id = counter.emission_texture++;
        }

        if(is(material,"emissiveFactor")){
            mesh.uniform.emission_factor = glm::vec3(
                material["emissiveFactor"][0],
                material["emissiveFactor"][1],
                material["emissiveFactor"][2]
            );
        }
    }

    //----------------------------------------------------

    std::vector<Mesh> build_meshes(uint32_t mesh_id, uint32_t depth = 0)
    {
        std::vector<Mesh> model_meshes;
        json mesh = content["meshes"][ mesh_id ];

        // primitive
        for(json primitive : mesh["primitives"])
        {   
            Mesh model_mesh;

            if(is(mesh, "name")){
                model_mesh.name = mesh["name"];
                model_mesh.name += " " + std::to_string(this->counter.mesh);
            } 

            // vertices, indices
            model_mesh.indices = create_indices(primitive);
            model_mesh.vertices = create_vertices(primitive);
            model_mesh.region = get_region(primitive);
            
            model_mesh.id = this->counter.mesh;
            model_mesh.ioffset = this->counter.indices;
            model_mesh.voffset = this->counter.vertices;

            this->counter.indices += model_mesh.indices.size();
            this->counter.vertices += model_mesh.vertices.size();
            
            gap(depth); msg::success(mesh["name"]," primitive");

            // material
            if(is(primitive, "material"))
            {
                uint32_t material_id = primitive["material"];
                fill_material_data(material_id, model_mesh ,depth);
            } 

            this->counter.mesh++;
            model_meshes.push_back(model_mesh);
        } // primitives

        return model_meshes;
    }
    //----------------------------------------------------
    /// Scene is made out of `nodes`, each node can have more nodes as children

    std::vector<Node> build_nodes(const json& nodes, int depth = 0)
    {
        std::vector<Node> model_nodes;
        for(uint32_t node_id: nodes)
        {
            Node model_node;
            json node = content["nodes"][node_id];

            if(is(node, "matrix"))
            {
                std::array<float, 16> nums = node["matrix"];
                model_node.cframe = glm::make_mat4(nums.data());
            }
            else
            {
                glm::vec3 translation = is(node, "translation")? glm::vec3( 
                    node["translation"][0],
                    node["translation"][1],
                    node["translation"][2]
                ) : glm::vec3(0);

                glm::quat rotation = is(node, "rotation")? glm::quat( 
                    node["rotation"][3],
                    node["rotation"][0],
                    node["rotation"][1],
                    node["rotation"][2]
                ) : glm::quat(1,0,0,0);

                glm::vec3 scale = is(node, "scale")? glm::vec3( 
                    node["scale"][0],
                    node["scale"][1],
                    node["scale"][2]
                ) : glm::vec3(1);

                model_node.cframe = construct_cframe(translation, rotation, scale);
            }

            model_node.name = is(node,"name")? node["name"] : "node";
            gap(depth); msg::highlight(model_node.name); // debug

            if(is(node,"mesh")){
                model_node.meshes = build_meshes(node["mesh"], depth+1);
            } 

            if(is(node,"children")){
                model_node.children = build_nodes(node["children"], depth+1);
            }

            model_nodes.push_back(model_node);
        } 
        
        return model_nodes;
    }

    //----------------------------------------------------
    //----------------------------------------------------

    Model load_glb(std::string path){ 
        
        // 0x46546C67 - file magic
        // 0x4E4F534A - JSON chunk type
        // 0x004E4942 - Bin chunk type 

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
        model.nodes = build_nodes(content["scenes"][0]["nodes"]);

        return model;
    }

    //----------------------------------------------------

    Model load_gltf(std::string path)
    {
        // remove file extension for other file reading
        std::size_t pos = path.find_last_of("/\\");
        this->name = path.substr( pos+1, path.length() - pos - 6 );
        this->folder = path.substr( 0, pos+1);

        // read json
        std::ifstream stream(path);
        stream >> content;
        stream.close();

        std::ofstream out("debug.json");
        out << content.dump(4); out.close();
        out.close();

        // read buffer (need multiple buffers)
        std::string buffer_uri = content["buffers"][0]["uri"];
        this->buffer = read_file(folder + buffer_uri);

        // build meshes
        Model model;
        model.nodes = build_nodes(content["scenes"][0]["nodes"]);

        return model;
    }

public:

    Model load(std::string path)
    {
        uint64_t start_time = timestamp_milli();
        Model model;
        std::string type;

        try{
            type = path.substr(path.length() - 3);
            if(type == "glb") {
                this->type = TYPE::GLB;
                model = load_glb(path);
            }

            type = path.substr(path.length() - 4);
            if(type == "gltf") {
                this->type = TYPE::GLTF;
                model = load_gltf(path);
            }

        }catch(const json::exception& e){
            msg::warn(std::string("Loader: ") + e.what());
            return Model();
        }catch(const std::exception& e){
            msg::warn(std::string("Loader: ") + e.what());
            return Model();
        };

        // clear
        model.total_indices_size = this->counter.indices;
        model.total_vertices_size = this->counter.vertices;
        this->counter.reset();

        msg::print("Time to create model: ", (float)(timestamp_milli() - start_time)/1000, "\n");
        return model;
    }
};
