in vec2 frag_UV;
in vec3 frag_Normal;
in vec4 frag_posLightSpace;
in vec3 frag_WorldPos;

uniform sampler2D uAlbedoTexture;
uniform sampler2D uARM;
uniform sampler2DShadow uShadowMap;
uniform sampler2D uNormalMap;

uniform vec3  uLightDirection;
uniform vec3  uLightColor;
uniform float uLightIntensity;

uniform vec3 uCameraPosition;
uniform float uUvScale;

out vec4 FragColor;

const float PI = 3.14159265359;



float computeShadow(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z >= 1.0)
        return 1.0;

    float bias = 0.0007;
    float shadow = 0.0;

    vec2 texelSize = 1.0 / textureSize(uShadowMap, 0);

    for (int x = -2; x <= 2; x++)
    for (int y = -2; y <= 2; y++)
    {
        shadow += texture(uShadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, projCoords.z - bias));
    }
    shadow /= 25.0;


    return shadow;
}


vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

void main()
{
    vec2 uv = frag_UV * uUvScale;

    vec3 albedo = pow(texture(uAlbedoTexture, uv).rgb, vec3(2.2));
    vec3 arm = texture(uARM, uv).rgb;

    float ao        = arm.r;
    float roughness = arm.g;
    float metallic  = arm.b;


    vec3 dp1 = dFdx(frag_WorldPos);
    vec3 dp2 = dFdy(frag_WorldPos);
    vec2 duv1 = dFdx(frag_UV);
    vec2 duv2 = dFdy(frag_UV);


    vec3 T = normalize(duv2.y * dp1 - duv1.y * dp2);
    vec3 B = normalize(-duv2.x * dp1 + duv1.x * dp2);

  
    vec3 Ngeom = normalize(cross(T, B));

    vec3 nmap = texture(uNormalMap, uv).xyz;
    nmap = nmap * 2.0 - 1.0;

    mat3 TBN = mat3(normalize(T), normalize(B), normalize(Ngeom));
    vec3 N = normalize(TBN * nmap);

    vec3 V = normalize(uCameraPosition - frag_WorldPos);
    vec3 L = normalize(-uLightDirection);
    vec3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);

    vec3 radiance = uLightColor * uLightIntensity;

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    vec3 numerator = NDF * G * F;
    float denom = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
    vec3 specular = numerator / denom;

    float shadow = computeShadow(frag_posLightSpace); 

    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL * shadow;

    vec3 ambient = albedo * ao * 0.3;  

    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
    //FragColor = vec4(vec3(shadow), 1.0);
}
