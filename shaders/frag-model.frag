#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inTexcoord;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inPosition;
layout(location = 3) in vec3 inViewPos;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D colorSampler;
layout(binding = 2) uniform sampler2D enviromentSampler;


vec2 SampleSphericalMap(vec3 v)
{
    const vec2 invAtan = vec2(0.1591, 0.3183);
    vec2 uv = vec2(atan(v.y, v.x), asin(-v.z));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}


void main() {
    //outColor = vec4(fragTexture, 0.0, 1.0);
    //outColor = vec4(fragColor, 1.0);
    //outColor = vec4(abs(fragColor) * texture(texSampler, fragTexture).rgb, 1.0);
    
    vec3 I = normalize(inPosition - inViewPos);
    vec3 R = reflect(I, normalize(inNormal));

    float ratio = 0;
    vec3 reflection_color = texture(enviromentSampler, SampleSphericalMap(R)).rgb * ratio;
    vec3 diffuse_color = texture(colorSampler, inTexcoord).rgb * (1- ratio);

    const float gamma = 1;
    const float exposure = 0.8;
  
    vec3 mapped = vec3(1.0) - exp(-reflection_color * exposure); // Exposure tone mapping
    mapped = pow(mapped, vec3(1.0 / gamma)); // Gamma correction 

    outColor = vec4(mapped + diffuse_color, 1.0);


    outColor = vec4(abs(inNormal), 1.0);
}
