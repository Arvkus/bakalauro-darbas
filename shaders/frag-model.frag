#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inTexcoord;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inPosition;
layout(location = 3) in vec3 inViewPos;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform Properties {
    float exposure;
} properties;

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

layout(binding = 3) uniform sampler2D enviroment_sampler;
layout(binding = 4) uniform sampler2D albedo_sampler[32];
layout(binding = 5) uniform sampler2D normal_sampler[32];
layout(binding = 6) uniform sampler2D material_sampler[32];
layout(binding = 7) uniform sampler2D emission_sampler[32];


vec2 sample_spherical_map(vec3 v)
{
    const vec2 invAtan = vec2(0.1591, 0.3183);
    vec2 uv = vec2(atan(v.z, v.x), -asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main() {
    // proprties
    float gamma = 1;
    float exposure = .3;
    
    float rough = mesh.roughness;
    float metal = mesh.metalliness;

    vec3 albedo = mesh.albedo_id == -1? mesh.base_color : texture(albedo_sampler[mesh.albedo_id], inTexcoord).rgb;
    vec3 material = mesh.material_id == -1? vec3(0, rough, metal) : texture(material_sampler[mesh.material_id], inTexcoord).rgb;

    vec3 light_color = vec3(1.0) - exp(-vec3(0.6) * exposure);  
    vec3 light_dir = normalize(inViewPos - inPosition); // light direction (from view)
    
    //-------------------------------
    // ambient color;
    vec3 ambient_color = light_color * 0.01;
    
    //diffuse color
    float brightness = max(dot(inNormal, light_dir), 0.0);
    vec3 diffuse_color = light_color * brightness;

    // specular color
    float specular_str = 1.0 - material.y; // roughness
    vec3 I = normalize(inViewPos - inPosition); // to what fragment camera is looking (direction)
    vec3 R = reflect(-light_dir, inNormal);
    float spec = pow(max(dot(I, R), 0.0), 64);
    vec3 specular_color = specular_str * spec * vec3(1.0);  

    // reflect color
    vec2 uv = sample_spherical_map( reflect(-I, inNormal) );
    vec3 reflection_color = texture(enviroment_sampler, uv).rgb;
    vec3 mapped = vec3(1.0) - exp(-reflection_color * exposure); // exposure tone mapping
    mapped = pow(mapped, vec3(1.0 / gamma)); // gamma correction 

    // combined
    vec3 color = (ambient_color + diffuse_color + specular_color) * albedo;
    //vec3 result = color * (1-metal) + (mapped * metal);

    vec3 result = color * (1-material.z) + (mapped * material.z);
    vec3 normc = texture(normal_sampler[mesh.normal_id], inTexcoord).rgb;
    outColor = vec4(normc, 1.0);
}
