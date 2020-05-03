#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexcoord;

layout(location = 0) out vec2 outTexcoord;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outPosition;
layout(location = 3) out vec3 outViewPos;

layout(binding = 0) uniform Camera {
    mat4 view;
    mat4 proj;
} camera;

layout(binding = 2) uniform Mesh {
    mat4 cframe;
    float roughness;
    float metalliness;
    int albedo_texture_id;
    int material_texture_id;
    vec3 base_color;
} mesh;

void main() {
    mat4 v = inverse(camera.view); // camera world space
    mat4 m = mesh.cframe; // model world space

    outTexcoord = inTexcoord;

    outNormal = mat3(mesh.cframe) * inNormal;  // transpose(inverse(material.model))
    outPosition = vec3( m * vec4(inPosition, 1.0) ); //vec3(m[3][0], m[3][1], m[3][2]);
    outViewPos = vec3(v[3][0], v[3][1], v[3][2]);

    gl_Position = camera.proj * camera.view * mesh.cframe * vec4(inPosition, 1.0);
}