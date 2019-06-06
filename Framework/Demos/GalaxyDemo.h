
#ifndef GALAXY_DEMO
#define GALAXY_DEMO

#include "../BaseDemo.h"
#include "ParticleEngine/PhysicsParticle.h"

class GalaxyDemo : public BaseDemo
{
public:
    virtual void Initialize();
    virtual void IputKey(unsigned int wParam);

protected:
    virtual bool IsUsingInteroperability() const;

private:
    void PositionSphere(slmath::vec4 *startPositions, int particlesCount);

    int m_ParticleToAdd;
    vrAccelerator m_Accelerator;
};


#endif // GALAXY_DEMO