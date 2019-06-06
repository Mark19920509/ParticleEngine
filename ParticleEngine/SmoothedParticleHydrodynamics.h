
#ifndef SMOOTHED_PARTTICLE_HYDRODYNAMICS
#define SMOOTHED_PARTTICLE_HYDRODYNAMICS


class Grid3D;
class ParticlesGPU;

#include <slmath\vec4.h>

struct SphParameters
{
    slmath::vec4 gravity;
    float m_GazConstant;
    float m_MuViscosity;
    float m_DeltaT;
    float m_Mass;
    float m_H;

    SphParameters():gravity(0.0f, -9.8f, 0.0f, 0.0f),
                    m_GazConstant(10.0f),
                    m_MuViscosity(10.0f),
                    m_DeltaT(1.0f / 60.0f),
                    m_Mass(10.0f),
                    m_H(1.0f)
                    {
                    }
};

class SmoothedParticleHydrodynamics
{

public:
    SmoothedParticleHydrodynamics(Grid3D *grid3D);
    ~SmoothedParticleHydrodynamics();
    
    void SetParticlesMass(float mass);
    void SetParticlesGazConstant(float gazConstant);
    void SetParticlesMuViscosityt(float muViscosity);

    const SphParameters& GetParameters() const;

    float *GetDensity() const;
    float *GetPreviousDensity() const;

    int GetParticlesCount() const;

    slmath::vec4 *GetParticleAccelerations() const;

    void Initialize(slmath::vec4 *positions, int positionsCount);
    void Simulate(slmath::vec4 *positions, int positionsCount);


private:
    void ReallocParticles(int particlesCount);
    void InitDensity();
    void ComputePressure();
    void ComputePressureBigRange();
    void ComputePressureQuery();
    void SwapDensityBuffer();

private:

    // References don't own these data
    slmath::vec4    *m_ParticlePositions;
    int             m_ParticlesCount;
    Grid3D          *m_Grid3D;

    // Specific SPH; data owned these data
    slmath::vec4    *m_Pressure;


    float           *m_Density;
    float           *m_PreviousDensity;
    
    SphParameters m_SphParameters;
};

#endif // SMOOTHED_PARTTICLE_HYDRODYNAMICS