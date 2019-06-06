#include "TestGrid.h"

#include <slmath/slmath.h>
#include "Utility/Timer.h"

void TestGrid::Initialize()
{
    m_BugFound = false;
    const float positionInGrid = 1.0f;
    int sideX = 10, sideY = 15, sideZ = 26;
    m_ParticlesCount = sideX * sideY * sideZ;
    m_Colors = new float[m_ParticlesCount];
    m_PositionsColors = new slmath::vec4[m_ParticlesCount];
    int index = 0;
    slmath::vec4 *startPositions = new slmath::vec4[m_ParticlesCount ];
    for (int i = 0; i < sideX; i++)
    {
        for (int j = 0; j < sideY; j++)
        {
            for (int k = 0; k < sideZ; k++)
            {
                assert(index < m_ParticlesCount );
                startPositions[index++] = slmath::vec4( float(i+0.5f) * positionInGrid, 
                                                        float(j+0.5f)  * positionInGrid, 
                                                        float(k+0.5f) * positionInGrid);
            }
        }
    }

    /*startPositions[0] = slmath::vec4(16.0f, 16.0f, 16.0f);
    startPositions[1] = slmath::vec4(-16.0f, -16.0f, -16.0f);*/

    m_PhysicsParticle.Initialize(reinterpret_cast<vrVec4 *>(startPositions), index);
    m_PhysicsParticle.CreateGrid(reinterpret_cast<vrVec4 *>(startPositions), index);

    slmath::vec4 acceleration(0.0f, -10.0f, 0.0f);
    m_PhysicsParticle.SetParticlesAcceleration(*reinterpret_cast<vrVec4 *>(&acceleration));

    Timer::GetInstance()->StartTimer();
    delete[]startPositions;

    m_Index = 0;
    m_Increment = 1;
    m_Time = 0;
}

void TestGrid::Release()
{
    BaseDemo::Release();
    delete [] m_Colors;
    delete []m_PositionsColors;
}


void TestGrid::Simulate(float /*deltaT*/)
{
    m_PhysicsParticle.CreateGrid();

    for (int i = 0; i < m_ParticlesCount ; i++)
    {
        m_Colors[i] = Compress(slmath::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    }

    

    m_Time += Timer::GetInstance()->StopTimer() / 1000.0f;

    // Stop m_Increment if bug has been found !
    if (m_Time > m_Increment && ! m_BugFound)
    {
        m_Increment += 0.001f;
        m_Index++;
        if (m_Index >= m_ParticlesCount)
        {
            m_Index = 0;
        }
    }

    int neighborsBuffer[1024];

    int neighborsCount = m_PhysicsParticle.GetNeighbors(m_Index,  neighborsBuffer, 1024);

    for (int j = 0; j < neighborsCount; j++)
    {
        m_Colors[neighborsBuffer[j]] = Compress(slmath::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    }
    m_Colors[m_Index] =  Compress(slmath::vec4(0.0f, 1.0f, 0.0f, 1.0f));
    Timer::GetInstance()->StartTimer();

    const float maxDistance = length(slmath::vec3(1.0f));

    for (int i = 0; i < m_ParticlesCount; i++)
    {
        slmath::vec4 otherPosition = *reinterpret_cast<const slmath::vec4*>(&m_PhysicsParticle.GetParticlePositions()[i]);
        slmath::vec4 currentPosition = *reinterpret_cast<const slmath::vec4*>(&m_PhysicsParticle.GetParticlePositions()[m_Index]);

        float distance = slmath::length(currentPosition - otherPosition);

        // Test only if no bug has been found
        if (distance <= maxDistance && ! m_BugFound)
        {
            bool closeNeighborFound = false;
            for (int j = 0; j < neighborsCount; j++)
            {
                closeNeighborFound |= neighborsBuffer[j] == i;
            }
            // m_BugFound = !closeNeighborFound;
            // assert(closeNeighborFound && "Bug in Grid3D a close particle has not be found !!");
        }
    }

}

slmath::vec4 *TestGrid::GetParticlePositions() const
{
    for (int i = 0; i < m_ParticlesCount; i++)
    {
        m_PositionsColors[i] = *reinterpret_cast<const slmath::vec4*>(&m_PhysicsParticle.GetParticlePositions()[i]);
        m_PositionsColors[i].w = m_Colors[i];
    }
    return m_PositionsColors;
}