#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inTexcoord;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inPosition;
layout(location = 3) in vec3 inViewPos;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform Material {
    float roughness;
    float metalliness;
	int is_diffuse_color;
    int texture_id;
	vec4 diffuse_color;
    mat4 transformation;
} material;

layout(binding = 2) uniform sampler2D colorSampler[32];
layout(binding = 3) uniform sampler2D enviromentSampler;

vec2 SampleSphericalMap(vec3 v)
{
    const vec2 invAtan = vec2(0.1591, 0.3183);
    vec2 uv = vec2(atan(v.z, v.x), -asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

 // proprties
const float gamma = 1;
const float exposure = 0.3;
float metallic = material.metalliness;
float roughness = material.roughness;

void main() {
    vec3 I = normalize(inPosition - inViewPos);
    vec3 R = reflect(I, normalize(inNormal));
    vec3 light_dir = -I;

    // specular color
    float spec = pow(max(dot(light_dir, R), 0.0), 256);
    vec3 specular_color = .5 * spec * vec3(1.0, 1.0, 1.0);  

    // surface color
    vec3 albedo = material.is_diffuse_color == 1? material.diffuse_color.xyz : texture(colorSampler[material.texture_id], inTexcoord).rgb;

    // reflection color
    vec2 uv = SampleSphericalMap(R);
    vec3 reflection_color = texture(enviromentSampler, uv).rgb;
    vec3 mapped = vec3(1.0) - exp(-reflection_color * exposure); // Exposure tone mapping
    mapped = pow(mapped, vec3(1.0 / gamma)); // Gamma correction 

    // diffuse color
    float brightness = max(dot(inNormal, light_dir), 0.0);
    vec3 diffuse_color = albedo * brightness;
    vec3 ambient_color = vec3(1.0, 1.0, 1.0) * 0.1;

    //outColor = vec4(mapped, 1.0);

    vec3 result = specular_color; //(ambient_color + diffuse_color + specular_color) * albedo;
    outColor = vec4(result, 1.0);
}