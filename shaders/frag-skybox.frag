#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inUVW;
layout(location = 1) in vec2 fragTexcoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform samplerCube enviromentSampler;


void main() {
    vec3 color = texture(enviromentSampler, inUVW).rgb;
    outColor = vec4(color, 1.0);
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