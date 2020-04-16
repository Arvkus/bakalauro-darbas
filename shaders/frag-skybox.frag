#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inUVW;
layout(location = 1) in vec2 fragTexcoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;
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
    vec2 uv = SampleSphericalMap(normalize(inUVW)); // make sure to normalize localPos
    vec3 color = texture(enviromentSampler, uv).rgb;

    const float gamma = 1;
    const float exposure = 0.05;
  
    vec3 mapped = vec3(1.0) - exp(-color * exposure); // Exposure tone mapping
    mapped = pow(mapped, vec3(1.0 / gamma)); // Gamma correction 
    
    outColor = vec4(mapped, 1.0);
}


/*
void main() {
    //outColor = vec4(fragTexture, 0.0, 1.0);
    //outColor = vec4(fragColor, 1.0);
    //outColor = vec4(abs(fragColor) * texture(texSampler, fragTexture).rgb, 1.0);

    vec3 color = texture(enviromentSampler, inUVW).rgb;
    outColor = vec4(color, 1.0);
}
*/