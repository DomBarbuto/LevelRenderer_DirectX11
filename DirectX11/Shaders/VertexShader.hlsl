#pragma pack_matrix(row_major)
// an ultra simple hlsl vertex shader

cbuffer CB_PerObject : register(b0)
{
    float4x4 wMatrix;
    float4x4 vMatrix;
    float4x4 pMatrix;
};

struct VERTEX_In
{
	float3 iPos		:	POSITION;
	float3 iUvw		:	UVCOORD;
    float3 iNrm		:	NORMDIR;
};

struct VERTEX_Out
{
	float4 oPos		:	SV_POSITION;
    float3 oUvw		:	UVCOORD;
	float3 oNrm		:	NORMDIR;
    float3 wPosition :  WORLD;
    float3 camWPosition : WORLDCAM;
};

VERTEX_Out main(VERTEX_In vIn)
{
	VERTEX_Out vOut;
    
    // Save world position (for lighting in PS)
    vOut.wPosition = mul(vIn.iPos, wMatrix);
    // Save normal position in world space (for lighting in PS)
    vOut.oNrm = normalize(mul(vIn.iNrm, wMatrix));
    // Save camera position in world space (for lighting in PS)
    vOut.camWPosition = -float3(vMatrix._m30, vMatrix._m31, vMatrix._m32);

    vOut.oPos = float4(vIn.iPos, 1.0f);
    vOut.oUvw = vIn.iUvw;  
	
	// Put vertex in homogenous space
    vOut.oPos = mul(vOut.oPos, wMatrix);
    vOut.oPos = mul(vOut.oPos, vMatrix);
    vOut.oPos = mul(vOut.oPos, pMatrix);
	
    // Put vertex normal in world space
	return vOut;
}