#include "common.hpp"

//-------------------------------------------------------------------
/// `VK_MAKE_VERSION()` number `i32` converts to `x.x.x` string
std::string version_to_string(uint32_t version)
{
	uint16_t patch = std::bitset<12>(version).to_ulong();
	uint8_t minor = version >> 12;
	uint8_t major = version >> 22;

	return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
}

//-------------------------------------------------------------------
/// read file and return vector buffer data (bytes)
std::vector<char> read_file(const std::string &filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error(std::string("failed to open file - " + filename));
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	std::cout << "Read file success: " << filename << std::endl;

	return buffer;
}

//-------------------------------------------------------------------

uint64_t timestamp_milli()
{
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

uint64_t timestamp_micro()
{
	using namespace std::chrono;
	return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
}

uint64_t timestamp_nano()
{
	using namespace std::chrono;
	return duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
}

//-------------------------------------------------------------------
/// Position, normal, texture
struct Vertex
{
	Vertex(){};
	Vertex(glm::vec3 position, glm::vec3 normal, glm::vec2 texture){
		this->position = position;
		this->normal = normal;
		this->texture = texture;
	};
	
	alignas(16) glm::vec3 position;
	alignas(16) glm::vec3 normal;
	alignas(8) glm::vec2 texture;

	alignas(16) glm::vec2 tangent;
	alignas(16) glm::vec2 bitangent;

	/// buffer binding indices, tells `attribute locations` what buffer to use
	static VkVertexInputBindingDescription get_binding_description() {
		VkVertexInputBindingDescription description = {};
		description.binding = 0;
		description.stride = sizeof(Vertex);
		description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return description;
	};

	/// location attributes for vertex shader
	static std::array<VkVertexInputAttributeDescription, 5> get_attribute_descriptions() { // vertex shader layout attributes
		std::array<VkVertexInputAttributeDescription, 5> descriptions = {};

		descriptions[0].binding = 0;
		descriptions[0].location = 0;
		descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		descriptions[0].offset = offsetof(Vertex, position);

		descriptions[1].binding = 0;
		descriptions[1].location = 1;
		descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		descriptions[1].offset = offsetof(Vertex, normal);

		descriptions[2].binding = 0;
		descriptions[2].location = 2;
		descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		descriptions[2].offset = offsetof(Vertex, texture);

		descriptions[3].binding = 0;
		descriptions[3].location = 3;
		descriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		descriptions[3].offset = offsetof(Vertex, tangent);

		descriptions[4].binding = 0;
		descriptions[4].location = 4;
		descriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
		descriptions[4].offset = offsetof(Vertex, bitangent);

		return descriptions;
	};
};

//-------------------------------------------------------------------

struct UniformCameraStruct {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct UniformPropertiesStruct{
	float exposure;
};

struct Region{
	Region(){};
	Region(glm::vec3 min, glm::vec3 max){
		this->min = min;
		this->max = max;
	}

	glm::vec3 min = glm::vec3(0);
	glm::vec3 max = glm::vec3(0);
};

// alignas(); // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap14.html#interfaces-resources-layout
// scalar - byte size (uint16 2, uint32 4, float 4, double 8)
// vec2 - 8
// vec3/4 - 16
// mat4 - 16



//-------------------------------------------------------------------
        
bool is_validation_layers_supported()
{
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

	std::vector<VkLayerProperties> available_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());
	//e VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT
	/*
	for(const VkLayerProperties& layer_info : available_layers){
		printf(layer_info.layerName);
		std::cout<<std::endl;
	}
	*/
	
	for(const char* required_layer_name : VALIDATION_LAYERS){
		bool layer_found = false;

		for(const VkLayerProperties& layer_info : available_layers){
			if(std::strcmp(layer_info.layerName, required_layer_name) == 0){
				layer_found = true;
			}
		}

		if(!layer_found){
			printf("%s - validation layer is not supported \n", required_layer_name);
			return false;
		}
	}

	return true;
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
// Console output functions

namespace msg{
	enum Color{
		Black=0,
		Blue=1,
		Green=2,
		Red=4,
		Purple=5,
		Orange=6,
		White=7,
		Grey=8,
		Lime=10,
		Cyan=11,
		Yellow=14,
	};

	namespace details{
		void output(const glm::quat& m){ std::cout<<std::setprecision(3)<<std::fixed<<"["<<m[0]<<" "<<m[1]<<" "<<m[2]<<" "<<m[3]<<"]"; }
		void output(const glm::vec4& m){ std::cout<<std::setprecision(3)<<std::fixed<<"["<<m[0]<<" "<<m[1]<<" "<<m[2]<<" "<<m[3]<<"]"; }
		void output(const glm::vec3& m){ std::cout<<std::setprecision(3)<<std::fixed<<"["<<m[0]<<" "<<m[1]<<" "<<m[2]<<"]"; }
		void output(const glm::vec2& m){ std::cout<<std::setprecision(3)<<std::fixed<<"["<<m[0]<<" "<<m[1]<<"]"; }

		void output(const glm::mat4& m){
			std::cout.precision(3);
			std::cout<<std::endl<<std::fixed;
			for(int i = 0; i < 4; i++){
				for(int j = 0; j < 4; j++){
					std::cout<<std::setw(7)<<m[j][i]<<" ";
				}
				std::cout<<std::endl;
			}
			std::cout<<std::endl;
		}

		void output(const glm::mat3& m){
			std::cout.precision(3);
			std::cout<<std::endl<<std::fixed;
			for(int i = 0; i < 3; i++){
				for(int j = 0; j < 3; j++){
					std::cout<<m[j][i]<<" ";
				}
				std::cout<<std::endl;
			}
			std::cout<<std::endl;
		}

		void output(const glm::mat2& m){
			std::cout.precision(3);
			std::cout<<std::endl<<std::fixed;
			for(int i = 0; i < 2; i++){
				for(int j = 0; j < 2; j++){
					std::cout<<m[j][i]<<" ";
				}
				std::cout<<std::endl;
			}
			std::cout<<std::endl;
		}

		void output(const  uint8_t& m){ std::cout<<m; }
		void output(const uint16_t& m){ std::cout<<m; }
		void output(const uint32_t& m){ std::cout<<m; }
		void output(const uint64_t& m){ std::cout<<m; }

		void output(const int& m){ std::cout<<m; }
		void output(const char& m){ std::cout<<m; }
		void output(const float& m){ std::cout<<std::setprecision(3)<<m; }
		void output(const double& m){ std::cout<<std::setprecision(3)<<m; }
		void output(const std::string& m){ std::cout<<m; }

		void output(const nlohmann::json& m){ std::cout<<m; } 
		void output(const char* m){ std::cout<<m; } 

		template<typename TYPE>
		void output(const std::vector<TYPE>& vector){
			std::cout<<"( ";
			for(const TYPE& m : vector){ output(m); std::cout<<" "; }
			std::cout<<")";
		}

	}

	//---------------------------------------

	void print(){}

	template< typename FIRST >
	void print( const FIRST& first)
	{
		details::output(first);
	}

	template< typename FIRST, typename... REST > 
	void print( const FIRST& first, const REST&... rest )
	{
		details::output(first);
		print(rest...) ;
	}

	//---------------------------------------

	void printl(){ std::cout<<std::endl; }

	template< typename FIRST >
	void printl( const FIRST& first)
	{
		details::output(first);
		std::cout<<std::endl;
	}

	template< typename FIRST, typename... REST > 
	void printl( const FIRST& first, const REST&... rest )
	{
		details::output(first);
		print(rest...) ;
		std::cout<<std::endl;
	}

	//---------------------------------------
	template< typename FIRST >
	void warn( const FIRST& first)
	{
		SetConsoleTextAttribute(H_CONSOLE, msg::Color::Orange);
		printl(first);
		SetConsoleTextAttribute(H_CONSOLE, msg::Color::White);
	}

	template< typename FIRST, typename... REST > 
	void warn( const FIRST& first, const REST&... rest )
	{
		SetConsoleTextAttribute(H_CONSOLE, msg::Color::Orange);
		print(first);
		warn(rest...);
		SetConsoleTextAttribute(H_CONSOLE, msg::Color::White);
	};
	//---------------------------------------
	template< typename FIRST >
	void error( const FIRST& first)
	{
		SetConsoleTextAttribute(H_CONSOLE, msg::Color::Red);
		printl(first);
		SetConsoleTextAttribute(H_CONSOLE, msg::Color::White);
	}

	template< typename FIRST, typename... REST > 
	void error( const FIRST& first, const REST&... rest )
	{
		SetConsoleTextAttribute(H_CONSOLE, msg::Color::Red);
		print(first);
		error(rest...);
		SetConsoleTextAttribute(H_CONSOLE, msg::Color::White);
	};
	//---------------------------------------
	template< typename FIRST >
	void success( const FIRST& first)
	{
		SetConsoleTextAttribute(H_CONSOLE, msg::Color::Green);
		printl(first);
		SetConsoleTextAttribute(H_CONSOLE, msg::Color::White);
	}

	template< typename FIRST, typename... REST > 
	void success( const FIRST& first, const REST&... rest )
	{
		SetConsoleTextAttribute(H_CONSOLE, msg::Color::Green);
		print(first);
		success(rest...);
		SetConsoleTextAttribute(H_CONSOLE, msg::Color::White);
	};
	//---------------------------------------
	void highlight(const std::string& msg, msg::Color color = msg::Color::Yellow)
	{
		SetConsoleTextAttribute(H_CONSOLE, color);
		details::output(msg);
		std::cout<<std::endl;
		SetConsoleTextAttribute(H_CONSOLE, msg::Color::White);
	}
}


//-------------------------------------------------------------------