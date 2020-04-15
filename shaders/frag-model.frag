#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexture;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D enviromentSampler;

void main() {
    //outColor = vec4(fragTexture, 0.0, 1.0);
    //outColor = vec4(fragColor, 1.0);
    //outColor = vec4(abs(fragColor) * texture(texSampler, fragTexture).rgb, 1.0);
    
    float ratio = 0.05;
    vec3 color = abs(fragColor)*ratio + texture(texSampler, fragTexture).rgb*(1-ratio);
    outColor = vec4(color, 1.0);



}