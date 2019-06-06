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
};

struct VSParticleDrawOut
{
    float3 lightVector  : TEXCOORD2;
    float4 position		: POSITION;
};

struct GSSphereDrawOut
{
    float2 tex			: TEXCOORD0;
    float3 normal		: NORMAL;
    float3 lightVector  : TEXCOORD2;
    float4 position		: SV_POSITION;
};


struct PSParticleDrawIn
{
    float2 tex			: TEXCOORD0;
    float3 normal		: NORMAL;
    float3 lightVector  : TEXCOORD2;
};


Texture2D		            g_txDiffuse;


SamplerState g_samLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};

cbuffer cb0
{
    row_major float4x4  g_mWorldViewProj;
    row_major float4x4  g_mInvView;
    float3              cameraPosition; 
};


//
// Vertex shader for drawing the point-sprite particles
//
VSParticleDrawOut VSSphereDraw(VSParticleIn input)
{
    VSParticleDrawOut output;
    output.position = input.position;

    float3 lightPosition = cameraPosition;
    float3 lightVector = lightPosition - input.position.rgb;
	
    output.lightVector = lightVector;

    return output;
}


[maxvertexcount(84)]
void GSSphereDraw(point VSParticleDrawOut input[1], inout TriangleStream<GSSphereDrawOut> SpriteStream)
{
    GSSphereDrawOut output;
    
    int lod = 6;
    int latitudeCount = lod;
    int longitudeCount = lod;

    float thetaInc = (2.0f * 3.1415926f) / float(longitudeCount);
    float theta = 0.0f;

    float lambdaInc = 3.1415926f / float(latitudeCount);
    float lambda = 0.0f;
    float lambdaNext = lambdaInc;              
   
       
    int pointsCount = 2 * longitudeCount;
    theta = 0.0f;
    float radius = input[0].position.a;
    float3 center = input[0].position.rgb;
    
    // Iterate through the latitude
    for(int j = 0; j < latitudeCount; j++)
    {
        // Iterate through the longitude
        for(int i = 0; i < longitudeCount + 1; i++)
        {
            float3 position;

            // Bottom point
            position = float3 (radius*cos(theta)*sin(lambda), radius*sin(theta)*sin(lambda), radius*cos(lambda));
            
            float3 normal = position;
            normal = normalize(normal);
            output.normal = normal;
            
            position = position + center;
            output.position = mul( float4(position,1.0), g_mWorldViewProj ); 
            
            output.tex = float2(float(i) / float(longitudeCount), float(j) / float(latitudeCount));
            output.lightVector = input[0].lightVector;
            
            SpriteStream.Append(output);
            
            // Top point
            position = float3 (radius*cos(theta)*sin(lambdaNext), radius*sin(theta)*sin(lambdaNext), radius*cos(lambdaNext));
            
            normal = position;
            normal = normalize(normal);
            output.normal = normal;
            
            position = position + center;
            output.position = mul( float4(position,1.0), g_mWorldViewProj ); 

            output.tex = float2(float(i) / float(longitudeCount), float(j + 1) / float(latitudeCount));
            output.lightVector = input[0].lightVector;

            SpriteStream.Append(output);
            
            theta += thetaInc;
        }
        
        SpriteStream.RestartStrip();
        
        theta = 0.0f;
        lambda = lambdaNext;
        lambdaNext = lambdaNext + lambdaInc;
    }
}


///////////////////////////////////////////////
// Pixel Shader light
///////////////////////////////////////////////
float4 PSSphereDraw(PSParticleDrawIn input) : SV_Target
{
	float4 outColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
	float4 texColor = g_txDiffuse.Sample(g_samLinear, input.tex);
	float3 vDiffuse = float3(0.0f, 0.0f, 0.0f);

	float  maxLightDistance = 80.0f;


    float3 lightVector = input.lightVector;
    float distLight = length(lightVector);
	
		
    if (distLight < maxLightDistance)
    {
        float4 lightColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
        float factorLight = 1.0f - (distLight*distLight) / (maxLightDistance * maxLightDistance);
		factorLight = factorLight * 1.5f;

        lightColor = lightColor * factorLight;
		
        lightVector = normalize(lightVector);
        // calculate the diffuse lighting
        vDiffuse = CalculateDiffuse(texColor.rgb, lightColor, input.normal, lightVector);
    }

	float3 AmbientLightColor   = float3(0.6f, 0.6f, 0.6f);
	float3 vAmbient = CalculateAmbient(texColor.rgb, AmbientLightColor);
	outColor.rgb = saturate(vAmbient + vDiffuse);
    float fogCoef = 1.0f;
	outColor.rgb = outColor.rgb * fogCoef + (1-fogCoef) * float3(69.f/255.f, 55.f/255.f, 201.f/255.f);


    return outColor;
}
