layout(points) in;
layout(points, max_vertices = 1) out; 

in vec2 v_gridCoords[];

uniform mat4 uVPLight;   
uniform mat4 uInvVPLight; 
uniform mat4 uVPCamera;   
uniform mat4 uMatrixWorldPosToWaterUV; 
uniform float uWaterSize; 

uniform sampler2D uHeightWater;      
uniform sampler2D uTerrainHeightMap; 
uniform sampler2D uWaterMask;        
uniform sampler2D uShadowMap;       

//voir pour ajouter matrice worldpostoterrainuv

float getTotalHeight(vec2 uv) {
    float h_terrain = texture(uTerrainHeightMap, uv).r; 
    float h_water = texture(uHeightWater, uv).r;
    return h_terrain + h_water;
}

vec3 getWaterNormal(vec2 uv) { 
    vec2 E = vec2(0.01, 0.0); // TODO modif pour utiliser scale
    float h0 = getTotalHeight(uv);
    float h1 = getTotalHeight(uv + E.xy);
    float h2 = getTotalHeight(uv + E.yx); 

    vec3 p0 = vec3(0.0, h0, 0.0);
    vec3 p1 = vec3(E.x, h1, 0.0);
    vec3 p2 = vec3(0.0, h2, E.x);

    return normalize(cross(p2 - p0, p1 - p0));
}

vec3 unproject(vec2 gridCoord, float z){
    vec4 clipPos = vec4(gridCoord, z, 1.0);
    vec4 worldPos = uInvVPLight * clipPos;
    return worldPos.xyz / worldPos.w; 
}

vec3 findWaterSurface(vec3 rayOrigin, vec3 rayDir) {
    vec3 p = rayOrigin;
    
    for(int i = 0; i < 30; i++) {
        vec4 localPos4 = uMatrixWorldPosToWaterUV * vec4(p, 1.0);
        vec3 localPos = localPos4.xyz;

        float u = (localPos.x + uWaterSize * 0.5) / uWaterSize;
        float v = (localPos.z + uWaterSize * 0.5) / uWaterSize;

        float heightAtUV = getTotalHeight(vec2(u,v));

        // nul mais normal
        if(p.y > heightAtUV) {
            p = p + rayOrigin * 1.0;
        }else if(p.y < heightAtUV){
            p = p - rayOrigin * 0.1;
        }
    }

    return p;
}

/*
void emit(vec3 position){
    gl_PointSize = 5.0;
    gl_Position = vec4(position, 1.0);
    EmitVertex();
    EndPrimitive();
}
*/


void main(){
    vec2 grid = v_gridCoords[0]; 
    vec2 texCoord = grid * 0.5 + 0.5; 

    float isWater = texture(uWaterMask, texCoord).r;
    if (isWater < 0.5) return; 


    vec3 p_near = unproject(grid, -1.0);
    vec3 p_far = unproject(grid, 1.0);
    vec3 ray_dir = normalize(p_far - p_near);

    vec3 p = unproject(grid, -1.0);

    vec3 Ps = findWaterSurface(p_near, ray_dir);
    gl_PointSize = 5.0;
    gl_Position = uVPCamera * vec4(Ps, 1.0);
    EmitVertex();
    EndPrimitive();



    return;

}


