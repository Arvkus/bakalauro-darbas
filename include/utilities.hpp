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
		throw std::runtime_error("failed to open file");
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
	
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texture;

	/// buffer binding indices, tells `attribute locations` what buffer to use
	static VkVertexInputBindingDescription get_binding_description() {
		VkVertexInputBindingDescription description = {};
		description.binding = 0;
		description.stride = sizeof(Vertex);
		description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return description;
	};

	/// location attributes for vertex shader
	static std::array<VkVertexInputAttributeDescription, 3> get_attribute_descriptions() { // vertex shader layout attributes
		std::array<VkVertexInputAttributeDescription, 3> descriptions = {};

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

		return descriptions;
	};
};

//-------------------------------------------------------------------

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

//-------------------------------------------------------------------
        
bool is_validation_layers_supported()
{
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

	std::vector<VkLayerProperties> available_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

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

namespace out{
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

	//-------------------

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
		printl(rest...) ;
		std::cout<<std::endl;
	}

	//---------------------------------------

	void warn(const std::string& msg)
	{
		SetConsoleTextAttribute(H_CONSOLE, out::Color::Orange);
		details::output(msg);
		std::cout<<std::endl;
		SetConsoleTextAttribute(H_CONSOLE, out::Color::White);
	};

	void error(const std::string& msg)
	{
		SetConsoleTextAttribute(H_CONSOLE, out::Color::Red);
		details::output(msg);
		std::cout<<std::endl;
		SetConsoleTextAttribute(H_CONSOLE, out::Color::White);
	};

	void success(const std::string& msg)
	{
		SetConsoleTextAttribute(H_CONSOLE, out::Color::Green);
		details::output(msg);
		std::cout<<std::endl;
		SetConsoleTextAttribute(H_CONSOLE, out::Color::White);
	}

	void highlight(const std::string& msg, out::Color color = out::Color::Yellow)
	{
		SetConsoleTextAttribute(H_CONSOLE, color);
		details::output(msg);
		std::cout<<std::endl;
		SetConsoleTextAttribute(H_CONSOLE, out::Color::White);
	}
}


//-------------------------------------------------------------------