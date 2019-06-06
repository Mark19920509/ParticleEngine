#include "ParticlesSpring.h"
#include "Utility/Timer.h"
#include <slmath/slmath.h>

void ParticlesSpring::AddSpring(const Spring& spring)
{
    m_SpringsList.push_back(spring);
}

void ParticlesSpring::Solve(slmath::vec4 *positions, int positionsCount)
{
    Timer::GetInstance()->StartTimerProfile();
    const int springCount = m_SpringsList.size();

    for (int i = 0; i < springCount; i++)
    {
        const Spring &spring = m_SpringsList[i];
        assert(spring.m_ParticleIndex1 < positionsCount);
        assert(spring.m_ParticleIndex2 < positionsCount);

        if (spring.m_ParticleIndex1 == -1 || spring.m_Distance == 0.0f)
        {
            continue;
        }
        
        slmath::vec4 &position1 = positions[spring.m_ParticleIndex1];
        slmath::vec4 &position2 = positions[spring.m_ParticleIndex2];

        slmath::vec4 separation = position2 - position1;

        float sqrDistance = dot(separation, separation);

        const float epsilon = 1e-4f;
        float distance = 1e-1f;
        if (sqrDistance < epsilon)
        {
            distance = sqrt (sqrDistance);
        }

        slmath::vec4 movingVector = (spring.m_Distance / distance - 1.0f) * 0.5f * separation ;

        position1 -= movingVector;
        position2 += movingVector;
    }

    Timer::GetInstance()->StopTimerProfile("Solve Spring");
}

void ParticlesSpring::Release()
{
    m_SpringsList.clear();
}

const Spring* ParticlesSpring::GetSprings() const
{
    return &m_SpringsList[0];
}

int ParticlesSpring::GetSpringsCount() const
{
    return m_SpringsList.size();
}