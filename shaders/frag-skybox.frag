#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inUVW;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform Properties {
    float exposure;
} properties;

layout(binding = 3) uniform sampler2D enviroment_sampler;

vec2 sample_spherical_map(vec3 v)
{
    const vec2 invAtan = vec2(0.1591, 0.3183);
    vec2 uv = vec2(atan(v.z, v.x), -asin(v.y)); // vec2(atan(v.y, v.x), -asin(v.z)); 
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main() {
    vec2 uv = sample_spherical_map(normalize(inUVW)); // make sure to normalize localPos
    vec3 color = texture(enviroment_sampler, uv).rgb;

    const float gamma = 1;
    const float exposure = 0.3; // inExposure;
  
    vec3 mapped = vec3(1.0) - exp(-color * exposure); // Exposure tone mapping
    mapped = pow(mapped, vec3(1.0 / gamma)); // Gamma correction 
    
    outColor = vec4(mapped, 1.0);
}