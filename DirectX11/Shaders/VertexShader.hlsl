// an ultra simple hlsl vertex shader
#pragma pack_matrix(row_major)


cbuffer CB_PerObject : register(b0)
{
    float4x4 vMatrix;
    float4x4 pMatrix;
};

struct VERTEX_In
{
	float3 PosL		    :	POSITION;
	float3 UV		    :	UVCOORD;
    float3 NormalL		:	NORMDIR;
    float4x4 wMatrix    :   WORLD;
    uint InstanceID     :   SV_InstanceID;
};

struct VERTEX_Out
{
	float4 PosH		    :	SV_POSITION;
    float3 UV		    :	UVCOORD;
	float3 NormalW		:	NORMDIR;
    float3 PositionW    :   WORLDPOS;
    float3 PositionW_Cam :  WORLDCAMPOS;
    uint InstanceID : ID;
};

VERTEX_Out main(VERTEX_In vIn)
{
	VERTEX_Out vOut;
    
    // Save world position (for lighting in PS)
    vOut.PositionW = mul(float4(vIn.PosL, 1.0f), vIn.wMatrix).xyz;
    // Save normal position in world space (for lighting in PS)
    vOut.NormalW = normalize(mul(vIn.NormalL, (float3x3) vIn.wMatrix));
    // Save camera position in world space (for lighting in PS)
    vOut.PositionW_Cam = -float3(vMatrix._m30, vMatrix._m31, vMatrix._m32);
    vOut.UV = vIn.UV;

    vOut.PosH = float4(vIn.PosL, 1.0f);
	
	// Put vertex in homogenous space
    vOut.PosH = mul(vOut.PosH, vIn.wMatrix);
    vOut.PosH = mul(vOut.PosH, vMatrix);
    vOut.PosH = mul(vOut.PosH, pMatrix);
	
    // Put vertex normal in world space
	return vOut;
}