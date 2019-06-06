#include "WaterDemo.h"

#include <stdlib.h>




void WaterDemo::Initialize()
{

const float spaceBetweenParticles = 0.8f;
#ifdef DEBUG
    const int length = 8;
    const int sideBox = 16;
#else
    const int length = 8;
    const int sideBox = 16;
#endif //DEBUG
    const int halfSideBox = sideBox / 2;
    const int particlesCount = length*sideBox * sideBox * sideBox ;
    slmath::vec4 *startPositions = new slmath::vec4[particlesCount];
    int index = 0;
    for (int i = 0; i < sideBox; i++)
    {
        for (int j = 0; j < length*sideBox; j++)
        {
            for (int k = 0; k < sideBox; k++)
            {

                float noiseI = float((rand() % 100 - 50) * 0.0001f);
                float noiseJ = float((rand() % 100 - 50) * 0.0001f);
                float noiseK = float((rand() % 100 - 50) * 0.0001f);

                assert(index < particlesCount );
                startPositions[index] = slmath::vec4( (i - halfSideBox + noiseI) * spaceBetweenParticles, 
                                                        (j + 5 + noiseJ) * spaceBetweenParticles, 
                                                        (k - halfSideBox + noiseK) * spaceBetweenParticles);


                startPositions[index].w = Compress(s_Colors[rand() % s_ColorCount]);

                index++;
            }
        }
    }

    m_PhysicsParticle.SetParticlesViscosity(10.0f);
    m_PhysicsParticle.SetParticlesGazConstant(100.0f);
    m_PhysicsParticle.SetParticlesMass(10.0f);
    slmath::vec4 gravity(0.0f, -10.0f, 0.0f);
    m_PhysicsParticle.SetParticlesAcceleration(*reinterpret_cast<vrVec4*>(&gravity));

    // Add limitation
    {
        const slmath::vec4 boxMin(-20.0f, 0.0f, -20.0f);
        const slmath::vec4 boxMax(20.0f, 128.0f, 20.0f);

        vrAabb aabb;

        aabb.m_Min = *reinterpret_cast<const vrVec4*>(&boxMin);
        aabb.m_Max = *reinterpret_cast<const vrVec4*>(&boxMax);

        m_PhysicsParticle.AddInsideAabb(aabb);
    }

    // Add spheres
    m_SpheresCount = 9;
    m_Spheres = new slmath::vec4[m_SpheresCount];
    int sphereIndex = 0;
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            vrSphere sphere;

            slmath::vec4 spherePosition((i+j*3) * -15.0f - 100.0f, -1.0f/*-20.0f*(i+j*3)-50.0f*/, j * 15.0f - 15.0f);
            sphere.m_Position = *reinterpret_cast<const vrVec4*>(&spherePosition);
            sphere.m_Radius = 5.0f;

            m_PhysicsParticle.AddOutsideSphere(sphere);

            m_Spheres[sphereIndex] = *reinterpret_cast<slmath::vec4*>(&spherePosition);
            m_Spheres[sphereIndex++].w = Compress(slmath::vec4(1.0f, 1.0f, 1.0f, sphere.m_Radius));
        }
    }

    // Add accelerator
    const slmath::vec4 acceleratorPosition(1.0f, 2.0f, 3.0f);
    vrAccelerator accelerator;
    accelerator.m_Position = *reinterpret_cast<const vrVec4*>(&acceleratorPosition);
    accelerator.m_Radius = 100.0f;

    const slmath::vec4 acceleratorDirection(0.0f, -10.0f, 0.0f);
    accelerator.m_Direction = *reinterpret_cast<const vrVec4*>(&acceleratorDirection);
    accelerator.m_Type = 1;

    m_PhysicsParticle.AddAccelerator(accelerator);

    // Parameter

    m_PhysicsParticle.SetDamping(0.99f);

    m_PhysicsParticle.SetEnableGridOnGPU(true);
    m_PhysicsParticle.SetEnableSPHAndIntegrateOnGPU(true);
    m_PhysicsParticle.SetEnableCollisionOnGPU(true);
    m_PhysicsParticle.SetEnableAcceleratorOnGPU(false);
    m_PhysicsParticle.SetIsUsingCPU(false);




    // Physics initialization
    m_PhysicsParticle.Initialize(reinterpret_cast<vrVec4*>(startPositions), particlesCount);
    delete [] startPositions;

}

void WaterDemo::Simulate()
{
    vrSphere *spheres = m_PhysicsParticle.GetOutsideSpheres();
    for (int i = 0; i < m_SpheresCount; i++)
    {
        // Get physics sphere data
        vrSphere &sphere = spheres[i];
        // Update physics
        sphere.m_Position.x += 0.1f;
        // Update renderer
        float colorAndSize = m_Spheres[i].w;
        m_Spheres[i] = *reinterpret_cast<slmath::vec4*>(&sphere);
        m_Spheres[i].w = colorAndSize;
    }
    BaseDemo::Simulate();
}

void WaterDemo::IputKey(unsigned int wParam)
{
    switch (wParam)
    {
        case 0x4D: // M
            m_PhysicsParticle.SetParticlesMass(m_PhysicsParticle.GetParticlesMass() + 1.0f);
            break;
        case 0x47: // G
            m_PhysicsParticle.SetParticlesGazConstant(m_PhysicsParticle.GetParticlesGazConstant() + 1.0f);
            break;
        case 0x56: // V
            m_PhysicsParticle.SetParticlesViscosity(m_PhysicsParticle.GetParticlesViscosity() + 1.0f);
            break;
    }
}


void WaterDemo::Release()
{
    BaseDemo::Release();
    delete []m_Spheres;
}

void WaterDemo::InitializeGraphicsObjectToRender(DX11Renderer &renderer)
{
    renderer.InitializeParticles(GetParticlesCount(), m_SpheresCount, 0);
}

void WaterDemo::Draw(DX11Renderer &renderer) const
{
    renderer.RenderSphere(m_Spheres, m_SpheresCount);
    renderer.Render(GetParticlePositions(), GetParticlesCount());
}

bool WaterDemo::IsUsingInteroperability() const
{
    return false;
}

