#include "VerletIntegration.h"
#include "Grid3D.h"
#include "Utility/Timer.h"


VerletIntegration::VerletIntegration() :    m_CommonAcceleration(slmath::vec4(0.0f, 0.0f, 0.0f, 0.0f)),
                                            m_ParticlesCount(0),
                                            m_NewProsition(NULL),
                                            m_ParticlePositions(NULL),
                                            m_Accelerations(NULL),
                                            m_ParticlePreviousPositions(NULL),
                                            m_DeltaT(1.0f / 60.0f),
                                            m_Damping(0.99f)
{
}
VerletIntegration::~VerletIntegration()
{
    delete[] m_NewProsition;
    delete[] m_ParticlePositions;
    delete[] m_Accelerations;
    delete[] m_ParticlePreviousPositions;
}

void VerletIntegration::Initialize(slmath::vec4* positions, int particlesCount)
{
    assert(particlesCount > 0);
    if (m_ParticlesCount != particlesCount)
    {
        ReallocParticles(particlesCount);
    }
    for (int i = 0; i < m_ParticlesCount; i++)
    {
        m_ParticlePositions[i] = m_ParticlePreviousPositions[i] = positions[i];
        /*m_Accelerations[i] = slmath::vec4(0.0f);
        m_NewProsition[i] = slmath::vec4(0.0f);*/
    }
}

void VerletIntegration::SetGrid3D(Grid3D *grid3D)
{
    m_Grid3D  = grid3D;
}

void VerletIntegration::SetCommonAcceleration(const slmath::vec4 &acceleration)
{
    m_CommonAcceleration = acceleration;
}

void VerletIntegration::SetDamping(float damping)
{
    m_Damping = damping;
}

slmath::vec4 *VerletIntegration::GetParticlePositions() const
{
    return m_ParticlePositions;
}

slmath::vec4 *VerletIntegration::GetParticlePreviousPositions() const
{
    return m_ParticlePreviousPositions;
}

int VerletIntegration::GetParticlesCount() const
{
    return m_ParticlesCount;
}


void VerletIntegration::AccumateAccelerations(slmath::vec4* accelerations, int particlesCount)
{
    assert(particlesCount == m_ParticlesCount);
    for (int i = 0 ; i < particlesCount; i++)
    {
        m_Accelerations[i] += accelerations[i];
    }
}


void VerletIntegration::Integration()
{
    Timer::GetInstance()->StartTimerProfile();
    const float dampingPlusOne = m_Damping + 1.0f;
    for (int i = 0 ; i < m_ParticlesCount; i++)
    {
        slmath::vec4 acceleration = m_Accelerations[i] + m_CommonAcceleration;
        m_ParticlePreviousPositions[i] = (m_ParticlePositions[i] * dampingPlusOne - 
                                    m_ParticlePreviousPositions[i] * m_Damping) +
                                    acceleration * m_DeltaT * m_DeltaT;

        m_Accelerations[i] = slmath::vec4(0.0f);
    }
    SwapPositionBuffer();
    Timer::GetInstance()->StopTimerProfile("Integrate");
}

void VerletIntegration::ContinuousIntegration()
{
    const float m_H = 1.0f;
    const int maxIndexesCount = 1024;
    int indexesOnTrajectory[maxIndexesCount ];
    Timer::GetInstance()->StartTimerProfile();
    const float dampingPlusOne = m_Damping + 1.0f;

    for (int i = 0 ; i < m_ParticlesCount; i++)
    {

        int index = m_Grid3D->GetParticleCellOrder()[i].m_ParticleIndex;

        assert(m_ParticlePreviousPositions[index].w == 0.0f);
        assert(m_ParticlePositions[index].w == 0.0f);
        assert(m_NewProsition[index].w == 0.0f);

        slmath::vec4 acceleration = m_Accelerations[index] + m_CommonAcceleration;
        slmath::vec4 newDesination = (m_ParticlePositions[index] * dampingPlusOne - 
                                    m_ParticlePreviousPositions[index] * m_Damping) +
                                    acceleration * m_DeltaT * m_DeltaT;

       slmath::vec4 trajectory = newDesination - m_ParticlePositions[index];


       int indexesOnTrajectoryCount = m_Grid3D->ComputeParticlesOnTrajectory(i, *(slmath::vec3*)&trajectory, 
                                                                        *(slmath::vec3*)&m_ParticlePositions[index],
                                                                        indexesOnTrajectory,
                                                                        maxIndexesCount);
        
       

        for (int j = 0 ; j < indexesOnTrajectoryCount; j++)
        {
            int indexOnTrajectory = indexesOnTrajectory[j];

            // doesn't collide against itself
            if (indexOnTrajectory == index)
                continue;

            // Hack just to test CCD theory same cloth don't collide
            const int particlesByClothCount  = m_ParticlesCount / 2;
            if (index / particlesByClothCount == indexOnTrajectory / particlesByClothCount )
                continue;

            slmath::vec4 normalTrajectory;
            slmath::safeNormalize(trajectory, normalTrajectory);

            slmath::vec4 separation = m_ParticlePositions[indexOnTrajectory] - newDesination;
            assert(separation.w == 0.0f);
            assert(trajectory.w == 0.0f);
            float projection = slmath::dot(separation, trajectory);
            
            // If the two particles are going further from each other, leave this loop
            if (projection < 0.0f)
                continue;
            

            slmath::vec4 pointOfImpact = m_ParticlePositions[index] + normalTrajectory * projection;

            slmath::vec4 smallestSeparation = pointOfImpact - m_ParticlePositions[indexOnTrajectory];

            assert(smallestSeparation.w == 0.0f);
            float sqrDistance = dot(smallestSeparation, smallestSeparation);
            
            // Imapct found
            if (sqrDistance < m_H*m_H)
            {
                slmath::vec4 directionToGo = slmath::normalize(separation);
                newDesination = m_ParticlePositions[indexOnTrajectory] - directionToGo * m_H;
            }
        }
        m_NewProsition[index] = newDesination;
        m_Accelerations[index] = slmath::vec4(0.0f);
    }

    slmath::vec4 *buffer = m_ParticlePreviousPositions;
    buffer = m_ParticlePreviousPositions;
    m_ParticlePreviousPositions = m_ParticlePositions;
    m_ParticlePositions = m_NewProsition;
    m_NewProsition = buffer;
    Timer::GetInstance()->StopTimerProfile("Continuous Integrate");
}


void VerletIntegration::SwapPositionBuffer()
{
   // Swap buffer
    slmath::vec4 *buffer = m_ParticlePreviousPositions;
    m_ParticlePreviousPositions = m_ParticlePositions;
    m_ParticlePositions = buffer;
}

void VerletIntegration::ReallocParticles(int particlesCount)
{
    assert(particlesCount > 0);
    if (m_ParticlePreviousPositions != NULL)
    {
        delete[] m_ParticlePositions;
        delete[] m_Accelerations;
        delete[] m_ParticlePreviousPositions;
        delete[] m_NewProsition;
    }

    m_ParticlesCount = particlesCount;
    m_ParticlePositions         = new slmath::vec4[m_ParticlesCount];
    m_ParticlePreviousPositions = new slmath::vec4[m_ParticlesCount];
    /*m_Accelerations             = new slmath::vec4[m_ParticlesCount];
    m_NewProsition              = new slmath::vec4[m_ParticlesCount];*/
}
