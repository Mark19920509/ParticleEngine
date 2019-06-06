#include "lightfuncs.fxh"

#define INPUT_PATCH_SIZE 1
#define OUTPUT_PATCH_SIZE 1


cbuffer cb0
{
    row_major   float4x4 g_mWorldViewProj;
    row_major   float4x4 g_mInvView;
    float3      cameraPosition;
};

struct VS_CONTROL_POINT_INPUT
{
    float4 position        : POSITION;
};

struct VS_CONTROL_POINT_OUTPUT
{
    float3 position     : POSITION;
    float3 lightVector  : TEXCOORD2;
    float4 color		: COLOR;
};

struct HS_CONSTANT_DATA_OUTPUT
{
    float Edges[4]             : SV_TessFactor;
    float Inside[2]            : SV_InsideTessFactor;
};

struct HS_OUTPUT
{
    float3 position     : BEZIERPOS;
    float3 lightVector  : TEXCOORD2;
    float4 color		: COLOR;
};

struct DS_OUTPUT
{
    float4 position         : SV_POSITION;
    float3 vWorldPos        : WORLDPOS;
    float3 vNormal          : NORMAL;
    float3 lightVector      : TEXCOORD2;
    float4 color		    : COLOR;
};



VS_CONTROL_POINT_OUTPUT TesselateVS( VS_CONTROL_POINT_INPUT Input )
{
    VS_CONTROL_POINT_OUTPUT Output;

    Output.position = Input.position.rgb;
    float3 lightPosition = cameraPosition;
    float3 lightVector = lightPosition - Input.position.rgb;
	
    Output.lightVector = lightVector;

    unsigned int color = asuint(Input.position.a);
    unsigned int r = (unsigned int) color & 0xFF;
    unsigned int g = (unsigned int)(color >> 8) & 0xFF;
    unsigned int b = (unsigned int)(color >> 16) & 0xFF;
    unsigned int size = (unsigned int)(color >> 24) & 0xFF;

    Output.color = float4(float(r) / 255.0f, float(g) / 255.0f, float(b) / 255.0f, float(size) / 5.0f);

    return Output;
}


HS_CONSTANT_DATA_OUTPUT ConstantHS( InputPatch<VS_CONTROL_POINT_OUTPUT, INPUT_PATCH_SIZE> ip,
                                          uint PatchID : SV_PrimitiveID )
{    
    HS_CONSTANT_DATA_OUTPUT Output;

    float distance = length(ip[0].lightVector);

    float radius = ip[0].color.a;

    float g_fTessellationFactor = clamp(1000.0f / distance * radius, 9.0f, 20.0f);

    float TessAmount = g_fTessellationFactor;

    Output.Edges[0] = Output.Edges[1] = Output.Edges[2] = Output.Edges[3] = g_fTessellationFactor;
    Output.Inside[0] = Output.Inside[1] = g_fTessellationFactor;

    return Output;
}


[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(OUTPUT_PATCH_SIZE)]
[patchconstantfunc("ConstantHS")]
HS_OUTPUT TesselateHS( InputPatch<VS_CONTROL_POINT_OUTPUT, INPUT_PATCH_SIZE> p, 
                    uint i : SV_OutputControlPointID,
                    uint PatchID : SV_PrimitiveID )
{
    HS_OUTPUT Output;
    Output.position = p[i].position;
    Output.lightVector = p[i].lightVector;
    Output.color = p[i].color;
    return Output;
}


[domain("quad")]
DS_OUTPUT TesselateDS(HS_CONSTANT_DATA_OUTPUT input, 
                    float2 UV : SV_DomainLocation,
                    const OutputPatch<HS_OUTPUT, OUTPUT_PATCH_SIZE> bezpatch )
{

    float3 WorldPos = bezpatch[0].position;

    float pi2 = 6.28318530;
    float pi = pi2/2;
    float radius = bezpatch[0].color.a;
	float fi = pi*UV.x;
    float theta = pi2*UV.y;
    float sinFi,cosFi,sinTheta,cosTheta;
	sincos( fi, sinFi, cosFi);
	sincos( theta, sinTheta,cosTheta);
    float3 normal = float3(sinFi*cosTheta, sinFi*sinTheta, cosFi);
	WorldPos = WorldPos + radius * normal;

    DS_OUTPUT Output;
    Output.position = mul( float4(WorldPos, 1), g_mWorldViewProj );
    Output.vWorldPos = WorldPos;
    Output.vNormal = normal;
    Output.lightVector = bezpatch[0].lightVector;
    Output.color       = bezpatch[0].color;

    return Output;    
}


float4 TesselatePS(DS_OUTPUT input) : SV_TARGET
{
	float4 outColor = float4(input.color.r, input.color.g, input.color.b, 1.0f);
	float4 texColor = outColor;
	float3 vDiffuse = float3(0.0f, 0.0f, 0.0f);

	float  maxLightDistance = 40.0f;


    float3 lightVector = input.lightVector;
    float distLight = length(lightVector);
	
		
   
    float4 lightColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float factorLight = 1.0f - (distLight*distLight) / (maxLightDistance * maxLightDistance);
	factorLight = clamp(factorLight, 0.2f, 2.0f);

    lightColor = lightColor * 0.5f; //factorLight;
		
    lightVector = normalize(lightVector);
    // calculate the diffuse lighting
    vDiffuse = CalculateDiffuse(texColor.rgb, lightColor, input.vNormal, lightVector);

	float3 AmbientLightColor   = float3(0.3f, 0.3f, 0.3f);
	float3 vAmbient = CalculateAmbient(texColor.rgb, AmbientLightColor);
	outColor.rgb = saturate(vAmbient + vDiffuse);
    float fogCoef = 1.0f;
	outColor.rgb = outColor.rgb * fogCoef + (1-fogCoef) * float3(69.f/255.f, 55.f/255.f, 201.f/255.f);


    return outColor;
}

