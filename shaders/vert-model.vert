#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexcoord;

layout(location = 0) out vec2 outTexcoord;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outPosition;
layout(location = 3) out vec3 outViewPos;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    outTexcoord = inTexcoord;
    outNormal = mat3(transpose(inverse(ubo.model))) * inNormal; // surface normal vector
    outPosition = vec3(ubo.model * vec4(inPosition, 1.0));  // model position

    /*
    mat3 rotation = mat3(ubo.model); // get only rotation
    outNormal = rotation * outNormal; // rotate current normal
    outNormal = normalize(outNormal); // get rid of scaling
    */
    

    mat4 m = inverse(ubo.view); // camera world space
    outViewPos = vec3(m[3][0], m[3][1], m[3][2]);

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
}