in vec2 frag_UV;
in vec3 frag_Tangent;
in vec3 frag_Bitangent;
in vec3 frag_Normal;
in vec4 frag_posLightSpace;
in vec3 frag_WorldPos;

uniform sampler2D uAlbedoTexture;
uniform sampler2D uARM;
uniform sampler2D uHeightMap; 
uniform sampler2D uNormalMap;
uniform sampler2D uShadowMap;

uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uLightIntensity;

uniform vec3 uCameraPosition;
uniform float uUvScale;

out vec4 FragColor;


const float PI = 3.14159265359;

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
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



void main()
{
    // Textures
    vec2 uv = frag_UV * uUvScale;


    vec3 albedo     = pow(texture(uAlbedoTexture, uv).rgb, vec3(2.2));
    vec3 normal     = frag_Normal;
    vec3 arm = texture(uARM, uv).rgb;

    float metallic  = arm.b;
    float roughness = arm.g;
    float ao        = arm.r;

    vec3 N = normalize(normal);
    vec3 V = normalize(uCameraPosition - frag_WorldPos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);

    vec3 lightPos = frag_WorldPos + uLightDirection * 1.0;

    // calculate per-light radiance
    vec3 L = normalize(lightPos - frag_WorldPos);
    vec3 H = normalize(V + L);
    float distance    = length(lightPos - frag_WorldPos);
    float attenuation = 1.0 / (distance);
    vec3 radiance     = uLightColor * attenuation;

    // cook-torrance brdf
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular     = numerator / denominator;

    // add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    

    vec3 ambient = vec3(0.60) * albedo * ao;
    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 0.0);
}

