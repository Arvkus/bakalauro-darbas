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
	float is_diffuse_color;
	vec4 diffuse_color;
} material;

layout(binding = 2) uniform sampler2D colorSampler;
layout(binding = 3) uniform sampler2D enviromentSampler;

vec2 SampleSphericalMap(vec3 v)
{
    const vec2 invAtan = vec2(0.1591, 0.3183);
    vec2 uv = vec2(atan(v.y, v.x), -asin(v.z));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}


void main() {
    vec3 I = normalize(inPosition - inViewPos);
    vec3 R = reflect(I, normalize(inNormal));

    float ratio = material.metalliness;
    vec2 uv = SampleSphericalMap(R);
    vec3 reflection_color = texture(enviromentSampler, uv).rgb * ratio;
    vec3 diffuse_color = texture(colorSampler, inTexcoord).rgb * (1- ratio);

    const float gamma = 1;
    const float exposure = 0.3;
  
    vec3 mapped = vec3(1.0) - exp(-reflection_color * exposure); // Exposure tone mapping

    // mapped = pow(mapped, vec3(1.0 / gamma)); // Gamma correction 

    if(material.is_diffuse_color == 1.0){
        outColor = vec4(mapped*material.metalliness + material.diffuse_color.xyz*(1-material.metalliness), 1.0);
    }else{
        outColor = vec4(mapped + diffuse_color, 1.0);
    }


    //outColor = vec4(abs(inNormal), 1.0);
}