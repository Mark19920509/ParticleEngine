#include "SimulationsTransition.h"


#ifdef DEBUG
    const int SimulationsTransition::m_SideCloth = 16;
    const int SimulationsTransition::m_ClothCount = 16;
#else
    const int SimulationsTransition::m_SideCloth = 16;
    const int SimulationsTransition::m_ClothCount = 16;
#endif //DEBUG


void SimulationsTransition::Initialize()
{

    const int sideClothMinusOne = m_SideCloth - 1;
    const int halfSideCloth = m_SideCloth / 2;
    const int particlesCount = m_SideCloth * m_SideCloth * m_ClothCount;
    const int spaceBetweenParticles = 3;
    m_StartPositions = new slmath::vec4[particlesCount];
    int index = 0;

    vrSpring nullSpring;

    nullSpring.m_Distance = 0.0f;
    nullSpring.m_ParticleIndex1 = 0;
    nullSpring.m_ParticleIndex2 = 0;

    for (int k = 0; k < m_ClothCount; k++)
    {
        const float spaceBetweenCloth = 60.0f;
        slmath::vec4 clothPosition((-k % (int)sqrt((float)m_ClothCount) + sqrt((float)m_ClothCount) * 0.5f) * spaceBetweenCloth,
                                    (100.0f + k*0.5f),
                                    (- k / sqrt((float)m_ClothCount) + sqrt((float)m_ClothCount) * 0.5f) * spaceBetweenCloth);
        
        for (int i = 0; i < m_SideCloth; i++)
        {
            for (int j = 0; j < m_SideCloth; j++)
            {
                assert(index < particlesCount);   
                float noise = 0.0f; // float((rand() % 100 - 50) * 0.0001f); 
                slmath::vec4 particlePosition(float(i - halfSideCloth) * spaceBetweenParticles + noise,
                                              0.0f,
                                              float(j - halfSideCloth) * spaceBetweenParticles + noise);

                m_StartPositions[index] = clothPosition + particlePosition;

                m_StartPositions[index].w = Compress(slmath::vec4(0.3f, 0.1f, 0.1f, 1.0f));
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
    

    const int particlesByCloth = particlesCount / m_ClothCount;
    const int halfParticlesByClothCount = particlesByCloth / 2;

    for (int k = 0; k < m_ClothCount; k++)
    {
        int firstParticleIndex = k * particlesByCloth;
        int lastParticleIndex = firstParticleIndex + particlesByCloth;
        for (int i = 0; i < halfParticlesByClothCount; i++)
        {
                
            // Structural constraints horizontal
            const int leftNeighborIndex = onePointOverTwoIndex - 1;
            if (leftNeighborIndex >= firstParticleIndex && leftNeighborIndex % m_SideCloth != sideClothMinusOne)
            {
                CreateSpring(onePointOverTwoIndex, leftNeighborIndex);
            }
            else
            {
                m_PhysicsParticle.AddSpring(nullSpring);
            }


            const int rightNeighborIndex = onePointOverTwoIndex + 1;
            if (rightNeighborIndex < lastParticleIndex && rightNeighborIndex % m_SideCloth != 0)
            {
                CreateSpring(onePointOverTwoIndex, rightNeighborIndex);
            }
            else
            {
                m_PhysicsParticle.AddSpring(nullSpring);
            }
                

            // Structural constraints vertical
            const int upNeighborIndex = oneLineOverTwoIndex - m_SideCloth;
            if (upNeighborIndex >= firstParticleIndex)
            {
                CreateSpring(oneLineOverTwoIndex, upNeighborIndex);
            }
            else
            {
                m_PhysicsParticle.AddSpring(nullSpring);
            }

            const int downNeighborIndex = oneLineOverTwoIndex + m_SideCloth;
            if (downNeighborIndex < lastParticleIndex)
            {
                CreateSpring(oneLineOverTwoIndex, downNeighborIndex);
            }
            else
            {
                m_PhysicsParticle.AddSpring(nullSpring);
            }

            // Shearing constraints
            // On the left

            const int leftUpNeighborIndex = upNeighborIndex - 1;
            if (leftUpNeighborIndex >= firstParticleIndex && leftUpNeighborIndex % m_SideCloth != sideClothMinusOne)
            {
                CreateSpring(oneLineOverTwoIndex, leftUpNeighborIndex);
            }
            else
            {
                m_PhysicsParticle.AddSpring(nullSpring);
            }

            const int leftDownNeighborIndex = downNeighborIndex - 1;
            if (leftDownNeighborIndex < lastParticleIndex && leftDownNeighborIndex % m_SideCloth != sideClothMinusOne)
            {
                CreateSpring(oneLineOverTwoIndex, leftDownNeighborIndex);
            }
            else
            {
                m_PhysicsParticle.AddSpring(nullSpring);
            }

            // Shearing on the right
            const int rightUpNeighborIndex = upNeighborIndex + 1;
            if (rightUpNeighborIndex >= firstParticleIndex && rightUpNeighborIndex % m_SideCloth != 0)
            {
                CreateSpring(oneLineOverTwoIndex, rightUpNeighborIndex);
            }
            else
            {
                m_PhysicsParticle.AddSpring(nullSpring);
            }

            const int rightDownNeighborIndex = downNeighborIndex + 1;
            if (rightDownNeighborIndex < lastParticleIndex && rightDownNeighborIndex % m_SideCloth != 0)
            {
                CreateSpring(oneLineOverTwoIndex, rightDownNeighborIndex);
            }
            else
            {
                m_PhysicsParticle.AddSpring(nullSpring);
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
                CreateSpring(onePointOverFourIndex, right2NeighborIndex);
            }
            else
            {
                m_PhysicsParticle.AddSpring(nullSpring);
            }

            const int right4NeighborIndex = right2NeighborIndex + 2;
            if (right4NeighborIndex < lastParticleIndex && 
                right2NeighborIndex / m_SideCloth == right4NeighborIndex / m_SideCloth)
            {
                CreateSpring(right2NeighborIndex, right4NeighborIndex);
            }
            else
            {
                m_PhysicsParticle.AddSpring(nullSpring);
            }

            // Bending from top to bottom
            const int down2NeighborIndex = oneRowOverFourIndex + 2 * m_SideCloth;
            if (right2NeighborIndex < lastParticleIndex && 
                oneRowOverFourIndex % m_SideCloth == down2NeighborIndex % m_SideCloth) // same column
            {
                CreateSpring(oneRowOverFourIndex, down2NeighborIndex);
            }
            else
            {
                m_PhysicsParticle.AddSpring(nullSpring);
            }

            const int down4NeighborIndex = down2NeighborIndex + 2 * m_SideCloth;
            if (down4NeighborIndex < lastParticleIndex && 
                down2NeighborIndex % m_SideCloth == down4NeighborIndex % m_SideCloth) // same column
            {
                CreateSpring(down2NeighborIndex, down4NeighborIndex);
            }
            else
            {
                m_PhysicsParticle.AddSpring(nullSpring);
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

    

    m_PhysicsParticle.SetParticlesGazConstant(100.0f);
    m_PhysicsParticle.SetParticlesMass(1.0f);
    slmath::vec4 gravity(0.0f, -10.0f, 0.0f);

    m_PhysicsParticle.SetParticlesAcceleration(*reinterpret_cast<vrVec4*>(&gravity));
    m_PhysicsParticle.SetDamping(0.99f);


    m_PhysicsParticle.SetEnableGridOnGPU(false);
    m_PhysicsParticle.SetEnableSPHAndIntegrateOnGPU(false);
    m_PhysicsParticle.SetEnableSpringOnGPU(true);
    m_PhysicsParticle.SetEnableCollisionOnGPU(true);
    m_PhysicsParticle.SetClothCount(m_ClothCount);
    m_PhysicsParticle.SetEnableAcceleratorOnGPU(true);

    m_PhysicsParticle.SetIsUsingCPU(false);


    // Add accelerator gravity
    const slmath::vec4 acceleratorPosition(1.0f, 2.0f, 3.0f);
    vrAccelerator accelerator;
    accelerator.m_Position = *reinterpret_cast<const vrVec4*>(&acceleratorPosition);
    accelerator.m_Radius = 100.0f;

    const slmath::vec4 acceleratorDirection(0.0f, -10.0f, 0.0f);
    accelerator.m_Direction = *reinterpret_cast<const vrVec4*>(&acceleratorDirection);
    accelerator.m_Type = 1;

    m_PhysicsParticle.AddAccelerator(accelerator);
    
    

    m_PhysicsParticle.Initialize(reinterpret_cast<vrVec4*>(m_StartPositions), particlesCount);

    delete[] m_StartPositions;

    // Add limitation
    {
        const slmath::vec4 boxMin(-500.0f, -10.0f, -500.0f);
        const slmath::vec4 boxMax(500.0f, 10000.0f, 500.0f);

        vrAabb aabb;

        aabb.m_Min = *reinterpret_cast<const vrVec4*>(&boxMin);
        aabb.m_Max = *reinterpret_cast<const vrVec4*>(&boxMax);

        m_PhysicsParticle.AddInsideAabb(aabb);
    }


    const float spaceBetweenSphere = 50.0f;
    m_SpheresCount = 0;
    m_Spheres = new slmath::vec4[25];
    int sphereIndex = 0;
    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 5; j++)
        {    
            m_SpheresCount++;
            vrSphere sphere;

            slmath::vec3 spherePosition(i * spaceBetweenSphere - 2.5f*spaceBetweenSphere, (i*3+j)*5.0f, j * spaceBetweenSphere - 2.5f*spaceBetweenSphere);
            sphere.m_Position = *reinterpret_cast<const vrVec4*>(&spherePosition);
            sphere.m_Radius = 20.0f;

            m_Spheres[sphereIndex] = *reinterpret_cast<slmath::vec4*>(&spherePosition);
            m_Spheres[sphereIndex++].w = Compress(slmath::vec4(1.0f, 1.0f, 1.0f, sphere.m_Radius - 1.0f));

            m_PhysicsParticle.AddOutsideSphere(sphere);

        }
    }

    m_FramesCount = 0;
}

void SimulationsTransition::CreateSpring(int index1, int index2)
{
    vrSpring spring;
    spring.m_ParticleIndex1 = index1;
    spring.m_ParticleIndex2 = index2;
    slmath::vec4 positionIndex1 = m_StartPositions[index1];
    positionIndex1.w = 0.0f;
    slmath::vec4 positionIndex2 = m_StartPositions[index2];
    positionIndex2.w = 0.0f;
    float distance = slmath::length(positionIndex1 - positionIndex2);
    spring.m_Distance = distance;
    m_PhysicsParticle.AddSpring(spring);
}

void SimulationsTransition::Release()
{
    BaseDemo::Release();
    delete []m_Spheres;
}

void SimulationsTransition::InitializeGraphicsObjectToRender(DX11Renderer &renderer)
{
    renderer.CreateIndexBuffersForCloth(m_SideCloth);
    renderer.InitializeParticles(GetParticlesCount(), m_SpheresCount, GetParticlesCount());
}


void SimulationsTransition::Draw(DX11Renderer &renderer) const
{
    
    if (m_FramesCount > 500)
    {
        renderer.Render(GetParticlePositions(), GetParticlesCount());
    }
    else
    {

        renderer.RenderSphere(&m_Spheres[0], m_SpheresCount);
  
        slmath::vec4 *pointerParticles = GetParticlePositions();
        for (int k = 0; k < m_ClothCount; k++)
        {
            renderer.RenderCloth(pointerParticles, GetParticlesCount() / m_ClothCount);
            pointerParticles += GetParticlesCount() / m_ClothCount;
        }
    }
}


void SimulationsTransition::Simulate()
{

    if (m_FramesCount == 0)
    {
        m_PhysicsParticle.SetEnableGridOnGPU(false);
        m_PhysicsParticle.SetEnableSPHAndIntegrateOnGPU(false);
        m_PhysicsParticle.SetEnableSpringOnGPU(false);
        m_PhysicsParticle.SetEnableCollisionOnGPU(true);
        m_PhysicsParticle.SetClothCount(m_ClothCount);
        m_PhysicsParticle.SetEnableAcceleratorOnGPU(true);
        m_PhysicsParticle.SetEnableSpringOnGPU(true);
    }

    if (m_FramesCount == 500)
    {
        m_PhysicsParticle.SetEnableSpringOnGPU(false);

        m_PhysicsParticle.ClearAccelerators();

        // Add accelerator force field
        const slmath::vec4 acceleratorPosition(0.0f, 0.0f, 0.0f);

        vrAccelerator accelerator;
        accelerator.m_Position = *reinterpret_cast<const vrVec4*>(&acceleratorPosition);
        accelerator.m_Radius = 1000.0f;

        const slmath::vec4 acceleratorDirection(0.0f, 1.0f, 0.0f);
        accelerator.m_Direction = *reinterpret_cast<const vrVec4*>(&acceleratorDirection);
        accelerator.m_Type = 0;

            m_PhysicsParticle.AddAccelerator(accelerator);
        

    }

    m_FramesCount++;
    BaseDemo::Simulate();
}