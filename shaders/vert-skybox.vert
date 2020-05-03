#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexcoord;

layout(location = 0) out vec3 outUVW;

layout(binding = 0) uniform Camera {
    mat4 view;
    mat4 proj;
} camera;

void main() {
    outUVW = inPosition;

    mat4 viewPos = mat4(mat3(camera.view));
    vec4 clipPos = camera.proj * viewPos * vec4(inPosition.xyz, 1.0);

    gl_Position = clipPos;
}