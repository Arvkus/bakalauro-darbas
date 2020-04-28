#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexcoord;

layout(location = 0) out vec2 outTexcoord;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outPosition;
layout(location = 3) out vec3 outViewPos;

layout(binding = 0) uniform Material {
    float roughness;
    float metalliness;
	int is_diffuse_color;
    int texture_id;
	vec4 diffuse_color;
    mat4 model; // model position
} material;

layout(binding = 1) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

vec3 center  = vec3(0.0);

void main() {
    mat4 v = inverse(ubo.view); // camera world space
    mat4 m = material.model; // model world space

    outTexcoord = inTexcoord;

    outNormal = mat3(material.model) * inNormal;  // transpose(inverse(material.model))
    outPosition = vec3( m * vec4(inPosition, 1.0) ); //vec3(m[3][0], m[3][1], m[3][2]);
    outViewPos = vec3(v[3][0], v[3][1], v[3][2]);

    gl_Position = ubo.proj * ubo.view * vec4(inPosition, 1.0);
}