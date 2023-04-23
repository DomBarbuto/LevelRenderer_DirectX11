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
};

cbuffer CB_PerScene : register(b2)
{
    OBJ_ATTRIBUTES atts[17];
}

struct VERTEX_In
{
    float4 PosH : SV_POSITION;
    float3 UV : UVCOORD;
    float3 NormalW : NORMDIR;
    float3 PositionW : WORLDPOS;
    float3 PositionW_Cam : WORLDCAMPOS;
};

static float4 ambientTerm = float4(0.75f, 0.75f, 0.75f, 1.0f);

float4 main(VERTEX_In vIn) : SV_TARGET
{
    // atts[vIn.InstanceID]
    float4 surfaceColor = float4(atts[matIndex.x].diffuseReflectivity, 1.0f);
    ////float4 surfaceColor = float4(0.5f, 0.5f, 0.5f, 1.0f); //TEST
    
    // Ambient and directional light
    float4 ambient = ambientTerm * surfaceColor;
    float4 directionalLight = saturate(dot(normalize(-directionalLightDir), vIn.NormalW))
                                * directionalLightColor * surfaceColor;
    float4 color = directionalLight + ambient; // Return color if you dont want specular
    
    // Specular
    float3 viewDir = normalize(vIn.PositionW_Cam - vIn.PositionW);
    float3 halfVec = normalize((-directionalLightDir) + viewDir);
    //float3 reflectVec = reflect(normalize(directionalLightDir), vIn.iNrm);
    float dotProduct = max(0.0f, dot(vIn.NormalW, halfVec));
    float intensity = pow(saturate(dotProduct), atts[matIndex.x].specularExponent);
    //float intensity = pow(saturate(dotProduct), 20); // delete
    
    // Done
    //return color;
    return color += (float4(atts[matIndex.x].specularReflectivity, 1.0f) * intensity);
    //return color += (float4(0.1f, 0.1f, 0.1f, 1.0f) * intensity);//DELETE
    ////return float4(halfVec, 1.0f);
    //return float4(vIn.NormalW, 1.0f); // Normal direction test
}