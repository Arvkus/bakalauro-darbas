#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexcoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec2 outTexcoord;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outPosition;
layout(location = 3) out vec3 outViewPos;
layout(location = 4) out outTangentSpace{
    mat3 TBN;
    vec3 viewPos;
    vec3 fragPos;
} tan_space;

layout(binding = 0) uniform Camera {
    mat4 view;
    mat4 proj;
} camera;

layout(binding = 2) uniform Mesh {
    mat4 cframe;
    vec3 base_color;
    vec3 emission_factor;
    float roughness;
    float metalliness;
    int albedo_id;
    int normal_id;
    int material_id;
    int emission_id;
} mesh;

void main() {
    mat4 v = inverse(camera.view); // camera world 
    
    vec3 T = normalize(vec3(mesh.cframe * vec4(inTangent,   0.0)));
    vec3 B = normalize(vec3(mesh.cframe * vec4(inBitangent, 0.0)));
    vec3 N = normalize(vec3(mesh.cframe * vec4(inNormal,    0.0)));
    mat3 TBN = transpose(mat3(T, B, N));

    outNormal = mat3(mesh.cframe) * inNormal;  // transpose(inverse(material.model))
    outPosition = vec3(mesh.cframe * vec4(inPosition, 1.0)); //vec3(m[3][0], m[3][1], m[3][2]);
    outViewPos = vec3(v[3][0], v[3][1], v[3][2]);

    tan_space.TBN = TBN;
    tan_space.viewPos = TBN * vec3(v[3][0], v[3][1], v[3][2]);
    tan_space.fragPos = TBN * vec3(mesh.cframe * vec4(inPosition, 0.0));

    outTexcoord = inTexcoord;

    gl_Position = camera.proj * camera.view * mesh.cframe * vec4(inPosition, 1.0);
}