
#include "ParticlesAccelerator.h"
#include "Utility/Timer.h"


ParticlesAccelerator::ParticlesAccelerator() :  m_AccelerationsCount(0),
                                                m_Accelerations(NULL)
{
}

ParticlesAccelerator::~ParticlesAccelerator()
{
    delete[] m_Accelerations;
}

void ParticlesAccelerator::AllocateAccelerations(int particlesCount)
{
    m_AccelerationsCount = particlesCount;
    if (m_Accelerations != NULL)
    {
        delete[] m_Accelerations;
    }
    m_Accelerations = new slmath::vec4[m_AccelerationsCount];
}

void ParticlesAccelerator::Initialize()
{
    for (int j = 0; j < m_AccelerationsCount; j++)
    {
        m_Accelerations[j] = slmath::vec4(0.0f);
    }
}

slmath::vec4 *ParticlesAccelerator::GetAccelerations()
{
    return m_Accelerations;
}

void ParticlesAccelerator::AddAccelerator(const Accelerator& accelerator)
{
    m_Accelerators.push_back(accelerator);
}

void ParticlesAccelerator::ClearAccelerators()
{
    m_Accelerators.clear();
}

const Accelerator *ParticlesAccelerator::GetAccelerators() const
{
    if (m_Accelerators.size() == 0)
    {
        return NULL;
    }

    return &m_Accelerators[0];
}

int ParticlesAccelerator::GetAcceleratorsCount() const
{
    return m_Accelerators.size();
}

void ParticlesAccelerator::UpdateAccelerator(int index, const Accelerator& accelerator)
{
    assert(static_cast<unsigned int>(index) < m_Accelerators.size());

    m_Accelerators[index] = accelerator;
}

void ParticlesAccelerator::Accelerate(slmath::vec4 *particles, int particlesCount)
{
    Timer::GetInstance()->StartTimerProfile();

    if (m_AccelerationsCount < particlesCount)
    {
        m_AccelerationsCount = particlesCount;
        if (m_Accelerations != NULL)
        {
            delete[] m_Accelerations;
        }
        m_AccelerationsCount = particlesCount;
        m_Accelerations = new slmath::vec4[m_AccelerationsCount];
        for (int j = 0; j < particlesCount; j++)
        {
            m_Accelerations[j] = slmath::vec4(0.0f);
        }
    }


    const int acceleratorCount = m_Accelerators.size();
    for (int i = 0; i < acceleratorCount; i++)
    {
        Accelerator accelerator = m_Accelerators[i];
        float sqrRadius = accelerator.m_Radius * accelerator.m_Radius;
        for (int j = 0; j < particlesCount; j++)
        {
            slmath::vec3 separation = accelerator.m_Position - slmath::vec3(particles[j]);
            float sqrDistance = slmath::dot(separation, separation);

            if (sqrDistance > 1.0f && sqrDistance < sqrRadius)
            {
                float distance = sqrt(sqrDistance);
                slmath::vec4 normal = separation / distance;

                m_Accelerations[j] = normal * (accelerator.m_Radius / distance);
            }
            else
            {
                m_Accelerations[j] = slmath::vec4(0.0f);
            }
        }
    }
    Timer::GetInstance()->StopTimerProfile("Accelerator");
}


void ParticlesAccelerator::Release()
{
    m_Accelerators.clear();
}