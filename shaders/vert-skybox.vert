#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexcoord;

layout(location = 0) out vec3 outUVW;
layout(location = 1) out vec2 outTexcoord;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    outUVW = inPosition;
    outTexcoord = inTexcoord;
    //* rotationX(3.1415926/2) 
    mat4 viewPos = mat4(mat3(ubo.view));
    vec4 clipPos = ubo.proj * viewPos * vec4(inPosition.xyz, 1.0);

    gl_Position = clipPos;
}