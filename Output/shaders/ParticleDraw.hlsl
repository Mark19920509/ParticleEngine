//--------------------------------------------------------------------------------------
// File: ParticleDraw.hlsl
//
// Shaders for rendering the particle as point sprite
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

struct VSParticleIn
{
    float4  position   : POSITION;
};

struct VSParticleDrawOut
{
    float4 color		: COLOR;
    float4 lightVec     : TEXCOORD2;
    float4 pos			: POSITION;
    
};

struct GSParticleDrawOut
{
    float2 tex			: TEXCOORD0;
    float4 color		: COLOR;
    float4 lightVec     : TEXCOORD2;
    float4 pos			: SV_POSITION;

};

struct PSParticleDrawIn
{
    float2 tex			: TEXCOORD0;
    float4 color		: COLOR;
    float4 lightVec     : TEXCOORD2;
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

cbuffer cb1
{
    static float g_fParticleRad = 0.3f;   
};

cbuffer cbImmutable
{
    static float3 g_positions[4] =
    {
        float3( -1, 1, 0 ),
        float3( 1, 1, 0 ),
        float3( -1, -1, 0 ),
        float3( 1, -1, 0 ),
    };
    
    static float2 g_texcoords[4] = 
    { 
        float2(0,0), 
        float2(0,1),
        float2(1,0),
        float2(1,1),
    };
};


//
// Vertex shader for drawing the point-sprite particles
//
VSParticleDrawOut VSParticleDraw(VSParticleIn input)
{
    VSParticleDrawOut output;
    output.pos = input.position;

    // Uncompress color and size
    unsigned int color = asuint(input.position.a);
    unsigned int r = (unsigned int) color & 0xFF;
    unsigned int g = (unsigned int)(color >> 8) & 0xFF;
    unsigned int b = (unsigned int)(color >> 16) & 0xFF;
    unsigned int size = (unsigned int)(color >> 24) & 0xFF;

    output.color = float4(float(r) / 255.0f, float(g) / 255.0f, float(b) / 255.0f, float(size) / 5.0f);

    

    float4 lightPosition = float4(cameraPosition, 0);
    float4 lightVector = lightPosition - input.position;
		
	output.lightVec = lightVector;
		

    return output;
}



//
// GS for rendering point sprite particles.  Takes a point and turns it into 2 tris.
//
[maxvertexcount(4)]
void GSParticleDraw(point VSParticleDrawOut input[1], inout TriangleStream<GSParticleDrawOut> SpriteStream)
{
    GSParticleDrawOut output;
    
    //
    // Emit two new triangles
    //
    for(int i=0; i < 4; i++)
    {
        float3 position = g_positions[i] * input[0].color.a;
        position = mul( position, (float3x3)g_mInvView ) + input[0].pos;
        output.pos = mul( float4(position, 1.0), g_mWorldViewProj ); 
        output.color = input[0].color;        
        output.lightVec = input[0].lightVec;
        output.tex = g_texcoords[i];
        SpriteStream.Append(output);
    }
    SpriteStream.RestartStrip();
}


//
// PS for drawing particles
//
float4 PSParticleDraw(PSParticleDrawIn input) : SV_Target
{   
    float4 outColor = g_txDiffuse.Sample( g_samLinear, input.tex ) * input.color;

    float distLight = length(input.lightVec);
    float maxLightDistance = 50000.0f;

    if (distLight < maxLightDistance)
    {
        float factorLight = 1.0f - (distLight*distLight) / (maxLightDistance * maxLightDistance);
        factorLight = factorLight * 1.5f;
        outColor.rgb = outColor.rgb * factorLight;
    }

    float3 ambientLight = float3(1.0f, 1.0f, 1.0f);
	 outColor.rgb = outColor.rgb * ambientLight;
    
    return outColor;
	 // return float4(0.0f, 0.0f, 0.3f, 1.0f);
}
