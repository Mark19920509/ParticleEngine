#include "RainDemo.h"

#include <stdlib.h>

#include <cmath>

void RainDemo::Initialize()
{


#ifdef DEBUG
    const int length = 64;
#else
    const int length = 1024;
#endif //DEBUG
    const int particlesCount = length*length;
    slmath::vec4 *startPositions = new slmath::vec4[particlesCount];
    

    const float spaceBetweenParticle = 10.0f;

    const float pi2 = 6.28318530f;
    const float pi = pi2/2;
	int index = 0;

    for (int i = 0; i < length; i++)
    {
        for (int j = 0; j < length; j++)
        {
            float x,y,z;
            float s = float(i) / float(length);
            float t = float(j) / float(length);

            x = 2.0f * (1.0f - std::powf(2.73f,(s * 6 * pi) / (6 * pi))) * std::cosf(s * 6 * pi) * std::cosf((t * 2 * pi) / 2) * std::cosf((t * 2 * pi) / 2);
            y = 2.0f * (-1.0f + std::powf(2.73f,(s * 6 * pi) / (6 * pi))) * std::sinf(s * 6 * pi) * std::cosf((t * 2 * pi) / 2) * std::cosf((t * 2 * pi) / 2);
            z = 1.0f - std::powf(2.73f,(s * 6 *pi) / (3 * pi)) - std::sinf(t * 2 * pi) + std::powf(2.73f,(s * 6 * pi) / (6 * pi)) * std::sinf(t * 2 *pi);
    
            
            startPositions[index] = slmath::vec4(x, 5-z , y, 0.0f) * spaceBetweenParticle;

            startPositions[index].w = Compress(s_Colors[index % s_ColorCount]);
            index++;
            
        }
    }



    m_PhysicsParticle.SetParticlesMass(10.0f);
    slmath::vec4 gravity(10.0f, 0.0f, 0.0f);
    m_PhysicsParticle.SetParticlesAcceleration(*reinterpret_cast<vrVec4*>(&gravity));
    m_PhysicsParticle.SetDamping(1.0f);

    m_PhysicsParticle.SetEnableAcceleratorOnGPU(true);
    m_PhysicsParticle.SetEnableCollisionOnGPU(true);
    m_PhysicsParticle.SetIsUsingCPU(false);

     // Add limitation
    {
        const slmath::vec4 boxMin(-1000.0f, -10.0f, -1000.0f);
        const slmath::vec4 boxMax(1000.0f, 1000.0f, 1000.0f);

        vrAabb aabb;

        aabb.m_Min = *reinterpret_cast<const vrVec4*>(&boxMin);
        aabb.m_Max = *reinterpret_cast<const vrVec4*>(&boxMax);

        m_PhysicsParticle.AddInsideAabb(aabb);
    }
    

    // Add sphere
    const float space = 100.0f;
    m_Spheres = new slmath::vec4[8];
    int sphereIndex = 0;
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            vrSphere sphere;

            slmath::vec4 spherePosition(i * space -space, 0.0f, j * space - space);
            sphere.m_Position = *reinterpret_cast<const vrVec4*>(&spherePosition);
            sphere.m_Radius = 25.0f;

            m_PhysicsParticle.AddOutsideSphere(sphere);

            m_Spheres[sphereIndex] = *reinterpret_cast<slmath::vec4*>(&spherePosition);
            m_Spheres[sphereIndex++].w = Compress(slmath::vec4(1.0f, 1.0f, 1.0f, sphere.m_Radius));
        }
    }

   // Add accelerator
    const slmath::vec4 acceleratorPosition(1.0f, 2.0f, 3.0f);
    m_Accelerator.m_Position = *reinterpret_cast<const vrVec4*>(&acceleratorPosition);
    m_Accelerator.m_Radius = 100.0f;

    const slmath::vec4 acceleratorDirection(0.0f, -10.0f, 0.0f);
    m_Accelerator.m_Direction = *reinterpret_cast<const vrVec4*>(&acceleratorDirection);
    m_Accelerator.m_Type = 1;

    m_PhysicsParticle.AddAccelerator(m_Accelerator);

    m_PhysicsParticle.Initialize(reinterpret_cast<vrVec4*>(startPositions), particlesCount);

    delete []startPositions;

}

void RainDemo::Release()
{
    BaseDemo::Release();
    delete []m_Spheres;
}

void RainDemo::InitializeGraphicsObjectToRender(DX11Renderer &renderer)
{
    renderer.InitializeParticles(GetParticlePositions(), GetParticlesCount(), 9, 0);
}

void RainDemo::Draw(DX11Renderer &renderer) const
{
    // First render sphere then particles
    renderer.RenderSphere(m_Spheres, 9);
    BaseDemo::Draw(renderer);
}

bool RainDemo::IsUsingInteroperability() const
{
    return true;
}
