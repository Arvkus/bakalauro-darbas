#include "common.hpp"

/// `VK_MAKE_VERSION()` number `i32` number converts to `x.x.x` string
std::string apiVersionToString(uint32_t version){

    uint16_t patch = std::bitset<12>(version).to_ulong();
    uint8_t minor = version >> 12;
    uint8_t major = version >> 22;

    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
}

std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    std::cout << "Read file success: " << filename << std::endl;

    return buffer;
}



uint64_t timeSinceEpochMilli() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

uint64_t timeSinceEpochMicro() {
  using namespace std::chrono;
  return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
}