
#ifndef TEST_GRID
#define TEST_GRID

#include "../BaseDemo.h"
#include "ParticleEngine/PhysicsParticle.h"

class TestGrid : public BaseDemo
{
public:
    virtual void Initialize();
    virtual void Release();
    virtual void Simulate(float deltaT);

    virtual slmath::vec4 *GetParticlePositions() const;

private:
    float           *m_Colors;
    slmath::vec4    *m_PositionsColors;
    int m_ParticlesCount;
    bool m_BugFound;

    float m_Increment;
    int m_Index;
    float m_Time;
};


#endif // TEST_GRID