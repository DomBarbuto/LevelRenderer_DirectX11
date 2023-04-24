// an ultra simple hlsl pixel shader
#pragma pack_matrix(row_major)

struct OBJ_ATTRIBUTES
{
    float3 diffuseReflectivity;     // Diffuse color
    float dissolveTransparency;
    float3 specularReflectivity;
    float specularExponent;
    float3 ambientReflectivity;
    float lReflectionMapSharpness;
    float3 transmissionFilter;
    float opticalDensity;           // Index of refraction
    float3 emissiveReflectivity;
    float illuminationModel;
};

// Needs to be 16 byte aligned
struct POINT_LIGHT
{
    float4x4 transform;
    float4 color;
    float energy;
    float distance;
    float q_attenuation;
    float l_attenuation;
};

// Needs to be 16 byte aligned
struct SPOT_LIGHT
{
    float4x4 transform;
    float4 color;
    float energy;
    float distance;
    float q_attenuation;
    float l_attenuation;
    float spotSize;
    float padding1, padding2, padding3;
};

cbuffer CB_PerObject : register(b0)
{
    float4x4 vMatrix;
    float4x4 pMatrix;
    float4 matIndex;
}

cbuffer CB_PerFrame : register(b1)
{
    float4 directionalLightColor;
    float3 directionalLightDir;
    float padding;
    POINT_LIGHT pointLights[16];
    SPOT_LIGHT spotLights[16];
};

cbuffer CB_PerScene : register(b2)
{
    OBJ_ATTRIBUTES atts[18];
    float numPointLights;
    float numSpotLights;
    float pad1;
    float pad2;
}

struct VERTEX_In
{
    float4 PosH : SV_POSITION;
    float3 UV : UVCOORD;
    float3 NormalW : NORMDIR;
    float3 PositionW : WORLDPOS;
    float3 PositionW_Cam : WORLDCAMPOS;
};

static float4 ambientTerm = float4(0.2f, 0.2f, 0.2f, 1.0f);

float4 main(VERTEX_In vIn) : SV_TARGET
{
    float4 surfaceColor = float4(atts[matIndex.x].diffuseReflectivity, 1.0f);
    
    // Ambient and directional light
    float4 ambient = ambientTerm * surfaceColor;
    float4 directionalLight = saturate(dot(normalize(-directionalLightDir), vIn.NormalW))
                                * directionalLightColor * surfaceColor;
    float4 color = directionalLight + ambient; // Return color if you dont want specular
    
    if (numPointLights > 0)
    {
        float4 pointLightColor = float4(0, 0, 0, 0);
       
        // For each point light in scene
        for (int i = 0; i < numPointLights; i++)
        {
            float3 lightWorldPos = pointLights[i].transform._41_42_43;
            
            //// Forget about this light if this pixel is outside point light distance/range
            //if (distance(lightWorldPos, vIn.PositionW) > pointLights[i].distance)
            //    continue;
            
            // Point light formula
            float3 lightDir = normalize(lightWorldPos - vIn.PositionW);
            float lightRatio = saturate(dot(normalize(lightDir), normalize(vIn.NormalW)));
            //float attenuation = pointLights[i].q_attenuation;
            float attenuation = 1.0f - saturate(length(lightWorldPos - vIn.PositionW) / pointLights[i].distance);
            attenuation *= attenuation;
            pointLightColor = lightRatio * pointLights[i].color * (pointLights[i].energy * 0.05f) * surfaceColor * attenuation;
        color += pointLightColor;
        }
        
    }
    
    //if(numSpotLights > 0)
    //{
    //    // For each spot light in scene
    //    for (int i = 0; i < numSpotLights; i++)
    //    {
            
    //    }
    //}
    
    // Specular
    float3 viewDir = normalize(vIn.PositionW_Cam - vIn.PositionW);
    float3 halfVec = normalize((-directionalLightDir) + viewDir);
    //float3 reflectVec = reflect(normalize(directionalLightDir), vIn.iNrm);
    float dotProduct = max(0.0f, dot(vIn.NormalW, halfVec));
    float specIntensity = pow(saturate(dotProduct), atts[matIndex.x].specularExponent);
    
    
    // Done
    return color += (float4(atts[matIndex.x].specularReflectivity, 1.0f) * specIntensity);
    //return color;                     // Return this if no specular
    //return float4(vIn.NormalW, 1.0f); // Normal direction test
}