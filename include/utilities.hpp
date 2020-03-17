#include "common.hpp"

/// `VK_MAKE_VERSION()` number `i32` converts to `x.x.x` string
std::string apiVersionToString(uint32_t version)
{
	uint16_t patch = std::bitset<12>(version).to_ulong();
	uint8_t minor = version >> 12;
	uint8_t major = version >> 22;

	return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
}

std::vector<char> readFile(const std::string &filename)
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

uint64_t timeSinceEpochMilli()
{
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

uint64_t timeSinceEpochMicro()
{
	using namespace std::chrono;
	return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
}

struct Vertex
{
	Vertex(glm::vec3 position, glm::vec3 normal, glm::vec2 texture){
		this->position = position;
		this->normal = normal;
		this->texture = texture;
	};
	
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texture;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription description = {};
		description.binding = 0;
		description.stride = sizeof(Vertex);
		description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return description;
	};

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() { // vertex shader layout attributes
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, normal);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texture);

		return attributeDescriptions;
	};
};