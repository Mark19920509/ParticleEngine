#include "GalaxyDemo.h"

#include <slmath/slmath.h>
#include <stdlib.h>

void GalaxyDemo::Initialize()
{

const int length = 4;
const float spaceBetweenParticles = 0.2f;
#ifdef DEBUG
    const int sideBox = 32;
#else
    const int sideBox = 64;
#endif //DEBUG

    slmath::vec4 colors[] = {   slmath::vec4(0.1f, 0.15f, 0.5f, 0.6f),
                                slmath::vec4(0.1f, 0.15f, 0.5f, 0.6f),
                                slmath::vec4(0.1f, 0.15f, 0.5f, 0.6f),
                                slmath::vec4(1.0f, 1.0f, 0.0f, 0.4f),
                                slmath::vec4(0.2f, 0.05f, 0.05f, 50.0f),
                                slmath::vec4(1.0f, 1.0f, 1.0f, 0.8f)
                            };


    const int halfSideBox = sideBox / 2;
    const int particlesCount = length * sideBox * sideBox * sideBox ;
    slmath::vec4 *startPositions = new slmath::vec4[particlesCount];
    int index = 0;
    for (int i = 0; i < sideBox; i++)
    {
        for (int j = 0; j < length*sideBox; j++)
        {
            for (int k = 0; k < sideBox; k++)
            {
                assert(index < particlesCount );
                startPositions[index] = slmath::vec4( (i - halfSideBox) * spaceBetweenParticles + float (rand() % 10) * 3.1f , 
                                                        (j - halfSideBox) * spaceBetweenParticles + float (rand() % 10) * 3.1f, 
                                                        (k - halfSideBox) * spaceBetweenParticles + float (rand() % 10) * 3.1f);

                startPositions[index] += slmath::vec4(float((rand() % 100 - 50) * 0.1f));


                startPositions[index].w =  Compress(colors[index % 3]);

                if (index % 1000 == 0)
                {
                    startPositions[index].w =  Compress(colors[5]);
                }

                if (index % 3001 == 0)
                {
                    startPositions[index].w =  Compress(colors[4]);
                }

                if (index % 203 == 0)
                {
                    startPositions[index].w =  Compress(colors[3]);
                }

                index++;
            }
        }
    }

   // PositionSphere(startPositions, particlesCount);

    m_PhysicsParticle.SetParticlesMass(10.0f);
    slmath::vec4 gravity(10.0f, 0.0f, 0.0f);
    m_PhysicsParticle.SetParticlesAcceleration(*reinterpret_cast<vrVec4*>(&gravity));
    m_PhysicsParticle.SetDamping(1.0f);

    m_PhysicsParticle.SetEnableAcceleratorOnGPU(true);
    m_PhysicsParticle.SetEnableGridOnGPU(false);
    m_PhysicsParticle.SetEnableSPHAndIntegrateOnGPU(false);
    m_PhysicsParticle.SetEnableSpringOnGPU(false);
    m_PhysicsParticle.SetEnableCollisionOnGPU(false);
    m_PhysicsParticle.SetIsUsingCPU(false);

    
    

   // Add accelerator
    for (int i = 0; i < 4; i++)
    {
        const slmath::vec4 acceleratorPosition((i/2)*200.0f - 100.0f, 0.0f, (i%2)*200.0f - 100.0f);
        // const slmath::vec4 acceleratorPosition(0.0f, 0.0f, 0.0f);

        m_Accelerator.m_Position = *reinterpret_cast<const vrVec4*>(&acceleratorPosition);
        m_Accelerator.m_Radius = 1000.0f;

        const slmath::vec4 acceleratorDirection(0.0f, 1.0f, 0.0f);
        m_Accelerator.m_Direction = *reinterpret_cast<const vrVec4*>(&acceleratorDirection);
        m_Accelerator.m_Type = vrAccelerator::ATTARCTION;

         m_PhysicsParticle.AddAccelerator(m_Accelerator);
    }


     // Initialize particles
     m_PhysicsParticle.Initialize(reinterpret_cast<vrVec4*>(startPositions), particlesCount);
     delete []startPositions;

     m_ParticleToAdd = 0;
}

void GalaxyDemo::IputKey(unsigned int wParam)
{

    const float speedAccelerator = 1.0f;
    const float radiusAccelerator = 100.0f;
    switch (wParam)
    {
        case VK_DOWN:
            m_Accelerator.m_Radius-= radiusAccelerator;
            break;
        case VK_UP:
            m_Accelerator.m_Radius += radiusAccelerator;
            break;
        case VK_RIGHT:
            m_Accelerator.m_Position.x += speedAccelerator;
            break;
        case VK_LEFT:
            m_Accelerator.m_Position.x -= speedAccelerator;
            break;
    }

    int acceleratorIndex = 0;
    m_PhysicsParticle.UpdateAccelerator(acceleratorIndex, m_Accelerator);
}


void GalaxyDemo::PositionSphere(slmath::vec4 *startPositions, int particlesCount)
{
    int lod = 400; // 260;
    int latitudeCount = lod;
    int longitudeCount = lod;

    float thetaInc = (2.0f * 3.1415926f) / float(longitudeCount);
    float theta = 0.0f;

    float lambdaInc = 3.1415926f / float(latitudeCount);
    float lambda = 0.0f;
    float lambdaNext = lambdaInc;              
   
    theta = 0.0f;
    float radius = 2.0f;
    const float constantRadius = 10.0f;
    slmath::vec4 center(0.0f, 0.0f, 0.0f);

    int index = 0;
    
    // Iterate through the latitude
    for(int j = 0; j < latitudeCount; j++)
    {
        // Iterate through the longitude
        for(int i = 0; i < longitudeCount; i++)
        {
            if (index + 2 >= particlesCount)
            {
                break;
            }

            // Bottom point
            startPositions[index] = slmath::vec4 (radius*cos(theta)*sin(lambda), radius*sin(theta)*sin(lambda), constantRadius*cos(lambda), 0.0f);
            startPositions[index] = startPositions[index] + center;
            
            index++;
            
            // Top point
            startPositions[index] = slmath::vec4 (radius*cos(theta)*sin(lambdaNext), radius*sin(theta)*sin(lambdaNext), constantRadius*cos(lambdaNext), 0.0f);
            startPositions[index] = startPositions[index] + center;
            theta += thetaInc;

            index++;

            radius += 0.001f;
        }        
        theta = 0.0f;
        lambda = lambdaNext;
        lambdaNext = lambdaNext + lambdaInc;
    }
}

bool GalaxyDemo::IsUsingInteroperability() const
{
    return true;
}
