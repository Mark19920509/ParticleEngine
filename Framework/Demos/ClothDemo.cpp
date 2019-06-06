#include "ClothDemo.h"
#include <process.h>


void ClothDemo::Initialize()
{

    m_SideCloth = 16;

#ifdef DEBUG
    m_ClothCount1 = 2048;
    m_ClothCount2 = 2048;
#else
    m_ClothCount1 = 2048;
    m_ClothCount2 = 2048;
#endif //DEBUG

    slmath::vec3 globalPosition1 = slmath::vec3(0.0f);
    CreateCloth(&m_PhysicsParticle, globalPosition1 , m_ClothCount1, false);

    slmath::vec3 globalPosition2 = slmath::vec3(0.0f, 0.0f, int(sqrt(float(m_ClothCount1)) + 2) * 20.0f);
    CreateCloth(&m_PhysicsParticle2, globalPosition2 ,m_ClothCount2, false);

    // Render accelerator
    m_Spheres = new slmath::vec4[0];

}

void ClothDemo::CreateCloth(PhysicsParticle *physicsParticle, const slmath::vec3& globalPosition, int clothCount, bool isUsingCPU)
{
    const int sideClothMinusOne = m_SideCloth - 1;
    const int halfSideCloth = m_SideCloth / 2;
    const int particlesCount = m_SideCloth * m_SideCloth * clothCount;
    float spaceBetweenParticlesI = 1;
    float spaceBetweenParticlesJ = 1;
    float spaceBetweenParticlesForRose[16] = {  9.0f, 12.25f, 13.5f, 15.0f,
                                                15.5f, 15.75f, 16.0f, 15.75f,
                                                15.50f, 15.25f, 15.0f, 10.0f,
                                                8.0f, 5.75f, 4.0f, 1.0f
                                             };
    const float spaceBetweenCloth = 20.0f;
    slmath::vec4 *startPositions = new slmath::vec4[particlesCount];
    int index = 0;

    vrSpring nullSpring;

    nullSpring.m_Distance = 0.0f;
    nullSpring.m_ParticleIndex1 = 0;
    nullSpring.m_ParticleIndex2 = 0;

    for (int k = 0; k < clothCount; k++)
    {
        slmath::vec4 clothPosition((50.0f /*+ k*0.5f*/), 
                                    (-k % (int)sqrt((float)clothCount) ) * spaceBetweenCloth,
                                    (- k / sqrt((float)clothCount)  ) * spaceBetweenCloth
                                    
                                    );

        clothPosition += globalPosition;
        
        spaceBetweenParticlesJ = 2;

        for (int i = 0; i < m_SideCloth; i++)
        {
            spaceBetweenParticlesJ += 0.5f;
            for (int j = 0; j < m_SideCloth; j++)
            {
                assert(index < particlesCount);
                slmath::vec4 particlePosition(float(i - halfSideCloth) * spaceBetweenParticlesI,
                                              0.0f,
                                              float(j - halfSideCloth) *spaceBetweenParticlesForRose[i] / 16.0f);

                startPositions[index] = clothPosition + particlePosition;

                if (i  == 0 && j >= 3 && j <= 13)
                {
                    startPositions[index].w = 0.0f;
                }
                else
                {
                    startPositions[index].w = 1.0f;
                }
                index++;
            }
        }
    }

    // For structural constraints only
    int onePointOverTwoIndex = 0;
    // For structural and shearing constraints
    int oneLineOverTwoIndex = 0;
    // For bending constraints
    int onePointOverFourIndex = 0;
    int oneRowOverFourIndex = 0;
    

    const int particlesByCloth = particlesCount / clothCount;
    const int halfParticlesByClothCount = particlesByCloth / 2;

    for (int k = 0; k < clothCount; k++)
    {
        int firstParticleIndex = k * particlesByCloth;
        int lastParticleIndex = firstParticleIndex + particlesByCloth;
        for (int i = 0; i < halfParticlesByClothCount; i++)
        {
                
            // Structural constraints horizontal
            const int leftNeighborIndex = onePointOverTwoIndex - 1;
            if (leftNeighborIndex >= firstParticleIndex && leftNeighborIndex % m_SideCloth != sideClothMinusOne)
            {
                CreateSpring(physicsParticle, onePointOverTwoIndex, leftNeighborIndex, startPositions);
            }
            else
            {
                physicsParticle->AddSpring(nullSpring);
            }


            const int rightNeighborIndex = onePointOverTwoIndex + 1;
            if (rightNeighborIndex < lastParticleIndex && rightNeighborIndex % m_SideCloth != 0)
            {
                CreateSpring(physicsParticle, onePointOverTwoIndex, rightNeighborIndex, startPositions);
            }
            else
            {
                physicsParticle->AddSpring(nullSpring);
            }
                

            // Structural constraints vertical
            const int upNeighborIndex = oneLineOverTwoIndex - m_SideCloth;
            if (upNeighborIndex >= firstParticleIndex)
            {
                CreateSpring(physicsParticle, oneLineOverTwoIndex, upNeighborIndex, startPositions);
            }
            else
            {
                physicsParticle->AddSpring(nullSpring);
            }

            const int downNeighborIndex = oneLineOverTwoIndex + m_SideCloth;
            if (downNeighborIndex < lastParticleIndex)
            {
                CreateSpring(physicsParticle, oneLineOverTwoIndex, downNeighborIndex, startPositions);
            }
            else
            {
                physicsParticle->AddSpring(nullSpring);
            }

            // Shearing constraints
            // On the left

            const int leftUpNeighborIndex = upNeighborIndex - 1;
            if (leftUpNeighborIndex >= firstParticleIndex && leftUpNeighborIndex % m_SideCloth != sideClothMinusOne)
            {
                CreateSpring(physicsParticle, oneLineOverTwoIndex, leftUpNeighborIndex, startPositions);
            }
            else
            {
                physicsParticle->AddSpring(nullSpring);
            }

            const int leftDownNeighborIndex = downNeighborIndex - 1;
            if (leftDownNeighborIndex < lastParticleIndex && leftDownNeighborIndex % m_SideCloth != sideClothMinusOne)
            {
                CreateSpring(physicsParticle, oneLineOverTwoIndex, leftDownNeighborIndex, startPositions);
            }
            else
            {
                physicsParticle->AddSpring(nullSpring);
            }

            // Shearing on the right
            const int rightUpNeighborIndex = upNeighborIndex + 1;
            if (rightUpNeighborIndex >= firstParticleIndex && rightUpNeighborIndex % m_SideCloth != 0)
            {
                CreateSpring(physicsParticle, oneLineOverTwoIndex, rightUpNeighborIndex, startPositions);
            }
            else
            {
                physicsParticle->AddSpring(nullSpring);
            }

            const int rightDownNeighborIndex = downNeighborIndex + 1;
            if (rightDownNeighborIndex < lastParticleIndex && rightDownNeighborIndex % m_SideCloth != 0)
            {
                CreateSpring(physicsParticle, oneLineOverTwoIndex, rightDownNeighborIndex, startPositions);
            }
            else
            {
                physicsParticle->AddSpring(nullSpring);
            }

            // Increment for structural and shearing
            onePointOverTwoIndex += 2;

            if (i % m_SideCloth == sideClothMinusOne)
            {
                oneLineOverTwoIndex += m_SideCloth + 1;
            }
            else
            {
                oneLineOverTwoIndex++;
            }
            

           // Bending constraints horizontal (from left to right)
            const int right2NeighborIndex = onePointOverFourIndex + 2;
            if (right2NeighborIndex < lastParticleIndex && 
                onePointOverFourIndex / m_SideCloth == right2NeighborIndex / m_SideCloth) // same row
            {
                CreateSpring(physicsParticle, onePointOverFourIndex, right2NeighborIndex, startPositions);
            }
            else
            {
                physicsParticle->AddSpring(nullSpring);
            }

            const int right4NeighborIndex = right2NeighborIndex + 2;
            if (right4NeighborIndex < lastParticleIndex && 
                right2NeighborIndex / m_SideCloth == right4NeighborIndex / m_SideCloth)
            {
                CreateSpring(physicsParticle, right2NeighborIndex, right4NeighborIndex, startPositions);
            }
            else
            {
                physicsParticle->AddSpring(nullSpring);
            }

            // Bending from top to bottom
            const int down2NeighborIndex = oneRowOverFourIndex + 2 * m_SideCloth;
            if (right2NeighborIndex < lastParticleIndex && 
                oneRowOverFourIndex % m_SideCloth == down2NeighborIndex % m_SideCloth) // same column
            {
                CreateSpring(physicsParticle, oneRowOverFourIndex, down2NeighborIndex, startPositions);
            }
            else
            {
                physicsParticle->AddSpring(nullSpring);
            }

            const int down4NeighborIndex = down2NeighborIndex + 2 * m_SideCloth;
            if (down4NeighborIndex < lastParticleIndex && 
                down2NeighborIndex % m_SideCloth == down4NeighborIndex % m_SideCloth) // same column
            {
                CreateSpring(physicsParticle, down2NeighborIndex, down4NeighborIndex, startPositions);
            }
            else
            {
                physicsParticle->AddSpring(nullSpring);
            }

            // Increment for bending
            if (i % 2 == 0)
            {
                onePointOverFourIndex++;
            }
            else
            {
                onePointOverFourIndex += 3;
            }

            if (i % (2 * m_SideCloth) != 2 * m_SideCloth - 1)
            {
                oneRowOverFourIndex++;
            }
            else
            {
                oneRowOverFourIndex += 2 * m_SideCloth + 1;
            }
        }
    }

    
    physicsParticle->SetParticlesGazConstant(100.0f);
    physicsParticle->SetParticlesMass(1.0f);
    slmath::vec4 gravity(0.0f, -10.0f, 0.0f);

    physicsParticle->SetParticlesAcceleration(*reinterpret_cast<vrVec4*>(&gravity));
    physicsParticle->SetDamping(0.99f);

    physicsParticle->SetEnableGridOnGPU(false);
    physicsParticle->SetEnableSPHAndIntegrateOnGPU(false);
    physicsParticle->SetEnableSpringOnGPU(true);
    physicsParticle->SetEnableCollisionOnGPU(false);
    physicsParticle->SetClothCount(clothCount);
    physicsParticle->SetEnableAcceleratorOnGPU(true);

    physicsParticle->SetIsUsingCPU(isUsingCPU);

    // Add accelerator
    const slmath::vec4 acceleratorPosition(32.0f, -8.0f, 4.0f);
    m_Accelerator.m_Position = *reinterpret_cast<const vrVec4*>(&acceleratorPosition);
    m_Accelerator.m_Radius = 10000.0f;

    const slmath::vec4 acceleratorDirection2(0.0f, -10.0f, 0.0f);
    m_Accelerator.m_Direction = *reinterpret_cast<const vrVec4*>(&acceleratorDirection2);
    m_Accelerator.m_Type = vrAccelerator::REPULSION;

    physicsParticle->AddAccelerator(m_Accelerator);

    // Initialize Physics
    physicsParticle->Initialize(reinterpret_cast<vrVec4*>(startPositions), particlesCount);
    delete[] startPositions;


}

void ClothDemo::CreateSpring(PhysicsParticle *physicsParticle, int index1, int index2, slmath::vec4 *startPositions)
{
    vrSpring spring;
    spring.m_ParticleIndex1 = index1;
    spring.m_ParticleIndex2 = index2;
    float distance = slmath::length(startPositions[index1] - startPositions[index2]);
    spring.m_Distance = distance;
    physicsParticle->AddSpring(spring);
}

void ClothDemo::IputKey(unsigned int wParam)
{

    const float speedAccelerator = 4.0f;
    switch (wParam)
    {
        case VK_PRIOR:
            m_Accelerator.m_Position.y += speedAccelerator;
            break;
        case VK_NEXT:
            m_Accelerator.m_Position.y -= speedAccelerator;
            break;
        case VK_UP:
            m_Accelerator.m_Position.x += speedAccelerator;
            break;
        case VK_DOWN:
            m_Accelerator.m_Position.x -= speedAccelerator;
            break;
        case VK_RIGHT:
            m_Accelerator.m_Position.z -= speedAccelerator;
            break;
        case VK_LEFT:
            m_Accelerator.m_Position.z += speedAccelerator;
            break;
    }

    slmath::vec4 sphere = *reinterpret_cast<slmath::vec4*>(&m_Accelerator.m_Position);
    sphere.w = Compress(slmath::vec4(0.1f, 0.1f, 0.5f, 10.0f));
    m_Spheres[0] = sphere;

    int acceleratorIndex = 0;
    m_PhysicsParticle.UpdateAccelerator(acceleratorIndex, m_Accelerator);
    m_PhysicsParticle2.UpdateAccelerator(acceleratorIndex, m_Accelerator);
}

void ClothDemo::Release()
{
    BaseDemo::Release();
    delete []m_Spheres;
}



unsigned int __stdcall SimulateOnCPU(PhysicsParticle* physicsParticle)
{
    physicsParticle->Simulate();
    _endthreadex(0);
    return 0;
}


void ClothDemo::Simulate(float /*delatT = 1 / 60.0f*/) 
{
    unsigned int threadId = 0;
    HANDLE runMutex = (HANDLE) _beginthreadex( NULL, 0, (unsigned int (__stdcall *)(void *))(&SimulateOnCPU), &m_PhysicsParticle2, 0, &threadId);

    m_PhysicsParticle.Simulate();

    WaitForSingleObject( runMutex, INFINITE );
    CloseHandle(runMutex);
}


void ClothDemo::InitializeGraphicsObjectToRender(DX11Renderer &renderer)
{
    m_PhysicsParticle2.InitializeOpenCL(renderer.GetID3D11Device(), renderer.GetID3D11Buffer());

    renderer.CreateIndexBuffersForCloth(m_SideCloth, slmath::max(m_ClothCount1, m_ClothCount2));

    int maxParticlesCount = slmath::max(GetParticlesCount(), m_PhysicsParticle2.GetParticlesCount());
    renderer.InitializeParticles(GetParticlePositions(), maxParticlesCount, 1, maxParticlesCount);
}


void ClothDemo::Draw(DX11Renderer &renderer) const
{
   renderer.RenderSphere(&m_Spheres[0], 1);

   //renderer.Render(GetParticlePositions(), GetParticlesCount());
   //renderer.Render(reinterpret_cast<slmath::vec4*>(m_PhysicsParticle2.GetParticlePositions()), 
   //         m_PhysicsParticle2.GetParticlesCount());

    renderer.RenderCloth(GetParticlePositions(), GetParticlesCount());
    renderer.RenderCloth(reinterpret_cast<slmath::vec4*>(m_PhysicsParticle2.GetParticlePositions()), 
            m_PhysicsParticle2.GetParticlesCount());

}

bool ClothDemo::IsUsingInteroperability() const
{
    return false;
}
