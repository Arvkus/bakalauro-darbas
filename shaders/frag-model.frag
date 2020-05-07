#version 450
#extension GL_ARB_separate_shader_objects : enable

const float PI = 3.14159265359;

//-----------------------------------------------------------------

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

//-----------------------------------------------------------------

vec2 sample_spherical_map(vec3 v)
{
    const vec2 invAtan = vec2(0.1591, 0.3183);
    vec2 uv = vec2(atan(v.z, v.x), -asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

//-----------------------------------------------------------------

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}  

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}   

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}
//-----------------------------------------------------------------

void main() {
    const float gamma = 1;
    const float exposure = .3;
    vec3 light_color = vec3(1.0);
    
    vec3 albedo = mesh.albedo_id == -1? mesh.base_color : texture(albedo_sampler[mesh.albedo_id], inTexcoord).rgb;
    albedo = pow(albedo, vec3(2.2));
    vec3 emission = mesh.emission_id == -1? vec3(0.0) : texture(emission_sampler[mesh.emission_id], inTexcoord).rgb;

    float ao = mesh.material_id == -1? 1.0 : texture(material_sampler[mesh.material_id], inTexcoord).r;
    float rough = mesh.material_id == -1? mesh.roughness : texture(material_sampler[mesh.material_id], inTexcoord).g;
    float metal = mesh.material_id == -1? mesh.metalliness : texture(material_sampler[mesh.material_id], inTexcoord).b;

    //vec3 light_dir = normalize(inViewPos - inPosition); // light direction (from view to fragment)
    //vec3 normal = inNormal;

    //-----------------
    vec3 Lo = vec3(0.0);
    vec3 N = normalize(inNormal); 
    vec3 V = normalize(inViewPos - inPosition);

    vec3 F0 = vec3(0.20); 
    F0 = mix(F0, albedo, metal);

    //-----------------
    // reflect color
    vec2 uv = sample_spherical_map( reflect(-V, N) );
    vec3 reflection_color = texture(enviroment_sampler, uv).rgb; //texture(enviroment_sampler, uv).rgb;
    vec3 mapped = vec3(1.0) - exp(-reflection_color * exposure); // exposure tone mapping
    //mapped = pow(mapped, vec3(1.0 / gamma)); // gamma correction 
    mapped = pow(mapped, vec3(2.2));

    //-----------------
    // loop start
    vec3 L = normalize(inViewPos - inPosition); // light
    vec3 H = normalize(V + L);

    float distance    = 5; //length(inViewPos - inPosition);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance     = light_color * attenuation;        
    
    // cook-torrance brdf
    float NDF = DistributionGGX(N, H, rough);        
    float G   = GeometrySmith(N, V, L, rough);      
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
    
    vec3 kS = F; // defraction (reflect)
    vec3 kD = vec3(1.0) - kS; // refraction
    kD = kD * (1.0 - metal);
    
    vec3 numerator    = NDF * G * F;
    float denominator = 32.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    vec3 specular     = numerator / max(denominator, 0.001);  
        
    // add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);                
    Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
    
    vec3 diffuse    =  albedo * 0.5;
    vec3 ambient    = (kD * diffuse) * ao; 
    vec3 color = ambient + Lo + kS*metal*mapped;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2)) * max(dot(N, V), 0.0);
   
    outColor = vec4(color + emission, 1.0);
}
