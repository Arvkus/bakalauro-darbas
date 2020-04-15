#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inTexcoord;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inPosition;
layout(location = 3) in vec3 inViewPos;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D colorSampler;
layout(binding = 2) uniform sampler2D enviromentSampler;



void main() {
    //outColor = vec4(fragTexture, 0.0, 1.0);
    //outColor = vec4(fragColor, 1.0);
    //outColor = vec4(abs(fragColor) * texture(texSampler, fragTexture).rgb, 1.0);
    
    vec3 I = normalize(inPosition - inViewPos);
    vec3 R = reflect(I, normalize(inNormal));

    float ratio = 0;
    vec3 reflection_color = texture(enviromentSampler, vec2(R.y, -abs(R.z))).rgb * ratio;
    vec3 diffuse_color = texture(colorSampler, inTexcoord).rgb * (1- ratio);

    outColor = vec4(reflection_color + diffuse_color, 1.0);
}