

#ifndef VERLET_INTEGRATION
#define VERLET_INTEGRATION

#include <slmath/slmath.h>

class Grid3D;

class VerletIntegration
{

public:
    VerletIntegration();
    ~VerletIntegration();
    
    int GetParticlesCount() const;
    slmath::vec4 *GetParticlePositions() const;
    slmath::vec4 *GetParticlePreviousPositions() const;
    void SetCommonAcceleration(const slmath::vec4 &acceleration);
    void SetDamping(float damping);
    void SetGrid3D(Grid3D *grid3D);

    void Initialize(slmath::vec4* positions, int particlesCount);
    void AccumateAccelerations(slmath::vec4* accelerations, int particlesCount);
    void Integration();
    void ContinuousIntegration();


    void SwapPositionBuffer();
private:
    void ReallocParticles(int particlesCount);
    

    slmath::vec4   *m_NewProsition;
    slmath::vec4   *m_ParticlePositions;
    slmath::vec4   *m_ParticlePreviousPositions;
    slmath::vec4   *m_Accelerations;
    slmath::vec4    m_CommonAcceleration;
    
    Grid3D          *m_Grid3D;

    float           m_DeltaT;
    float           m_Damping;
    int             m_ParticlesCount;
};


#endif // VERLET_INTEGRATION