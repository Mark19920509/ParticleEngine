//--------------------------------------------------------------------------------------
// File: ParticleDraw.hlsl
//
// Shaders for rendering the particle as point sprite
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "lightfuncs.fxh"

struct VSParticleIn
{
    float4  position   : POSITION;
    float4  normal     : NORMAL;
};

struct VSParticleDrawOut
{
    float3 normal		: NORMAL;
    float3 lightVector  : TEXCOORD2;
    float4 position		: SV_POSITION;
};


struct PSParticleDrawIn
{
    float3 normal		: NORMAL;
    float3 lightVector  : TEXCOORD2;
};


cbuffer cb0
{
    row_major   float4x4 g_mWorldViewProj;
    row_major   float4x4 g_mInvView;
    float3      cameraPosition;
    float3      lightPosition;
};


//
// Vertex shader for drawing the point-sprite particles
//
VSParticleDrawOut VSDraw(VSParticleIn input)
{
    VSParticleDrawOut output;
    
    output.position = mul( float4(input.position.rgb,1.0), g_mWorldViewProj );


    float3 lightVector = lightPosition - input.position.rgb;
	
    output.lightVector = lightVector;
    output.normal =  normalize(input.normal.rgb);

    return output;
}



///////////////////////////////////////////////
// Pixel Shader light
///////////////////////////////////////////////
float4 PSDraw(PSParticleDrawIn input) : SV_Target
{
	float4 outColor = float4(1.0f, 0.0f, 0.0f, 1.0f);
	float3 vDiffuse = float3(0.0f, 0.0f, 0.0f);

	float  maxLightDistance = 3000.0f;


    float3 lightVector = input.lightVector;
    float distLight = length(lightVector);
	
	float4 lightColor = float4(1.0f, 1.0f, 1.0f, 1.0f);	
    if (distLight < maxLightDistance)
    {
        float factorLight = 1.0f - (distLight*distLight) / (maxLightDistance * maxLightDistance);
		// factorLight = factorLight * 1.5f;

        lightColor = lightColor * factorLight;
		
        lightVector = normalize(lightVector);
        // calculate the diffuse lighting
        
        vDiffuse = outColor.rgb * lightColor * max(saturate(dot(input.normal, lightVector)), saturate(dot(-input.normal, lightVector)));
    }

    /*float3 sunLight = (-1.0f, -1.0f, 0.3f);
    sunLight = normalize(sunLight);
    float3 sunDiffuse = outColor.rgb * lightColor * max(saturate(dot(input.normal, sunLight)), saturate(dot(-input.normal, sunLight)));*/

	float3 AmbientLightColor   = float3(0.1f, 0.1f, 0.1f);
	float3 vAmbient = CalculateAmbient(outColor.rgb, AmbientLightColor);
	outColor.rgb = saturate(vAmbient + vDiffuse * 1.0f /*+ sunDiffuse*/);
    float fogCoef = 1.0f;
	outColor.rgb = outColor.rgb * fogCoef + (1-fogCoef) * float3(69.f/255.f, 55.f/255.f, 201.f/255.f);


    return outColor;
}

