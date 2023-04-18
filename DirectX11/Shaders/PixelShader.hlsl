// an ultra simple hlsl pixel shader
#pragma pack_matrix(row_major)

struct VERTEX_In
{
    float4 iPos : SV_Position;
    float3 iUvw : UVCOORD;
    float3 iNrm : NORMDIR;
    float3 wPosition : WORLD;
    float3 camWPosition : WORLDCAM;
};

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
    float4x4 wMatrix;
    float4x4 vMatrix;
    float4x4 pMatrix;
    OBJ_ATTRIBUTES objAtt;
};

cbuffer CB_PerFrame : register(b1)
{
    float4 directionalLightColor;
    float3 directionalLightDir;
    float padding;
};

static float4 ambientTerm = float4(0.25f, 0.25f, 0.25f, 1.0f);

float4 main(VERTEX_In vIn) : SV_TARGET
{
    float4 surfaceColor = float4(objAtt.diffuseReflectivity, 1.0f);
    
    // Ambient and directional light
    float4 ambient = ambientTerm * surfaceColor;
    float4 directionalLight = saturate(dot(normalize(-directionalLightDir), vIn.iNrm))
                                * directionalLightColor * surfaceColor;
    float4 color = directionalLight + ambient; // Return color if you dont want specular
    
    // Specular
    float3 viewDir = normalize(vIn.camWPosition - vIn.wPosition);
    float3 halfVec = normalize((-directionalLightDir) + viewDir);
    //float3 reflectVec = reflect(normalize(directionalLightDir), vIn.iNrm);
    float dotProduct = max(0.0f, dot(vIn.iNrm, halfVec));
    float intensity = pow( saturate(dotProduct), objAtt.specularExponent);
    
    // Done
    //return color;
    return color += (float4(objAtt.specularReflectivity, 1.0f) * intensity);
    //return float4(halfVec, 1.0f);
    //return float4(vIn.iNrm, 1.0f); // Normal direction test
}