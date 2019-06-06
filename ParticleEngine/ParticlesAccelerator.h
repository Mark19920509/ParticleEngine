
#ifndef PARTICLES_ACCELERATOR
#define PARTICLES_ACCELERATOR

#include <vector>
#include <slmath/slmath.h>
#include "Utility/AlignmentAllocator.h"


struct Accelerator
{
    slmath::vec4    m_Position;
    slmath::vec4    m_Direction;
    float           m_Radius;
    unsigned int    m_Type;
};

class ParticlesAccelerator
{
public:
    ParticlesAccelerator();
    ~ParticlesAccelerator();
    void Initialize();
    void AllocateAccelerations(int particlesCount);
    slmath::vec4 *GetAccelerations();
    void AddAccelerator(const Accelerator& accelerator);
    void ClearAccelerators();
    void Accelerate(slmath::vec4 *particles, int particlesCount);
    void Release();

    const Accelerator *GetAccelerators() const;
    int GetAcceleratorsCount() const;

    void UpdateAccelerator(int index, const Accelerator& accelerator);


private:
    // std::vector<Accelerator>    m_Accelerators;
    std::vector<Accelerator, AlignmentAllocator<Accelerator, 16> > m_Accelerators;
    slmath::vec4               *m_Accelerations;
    int                         m_AccelerationsCount;
};

#endif // PARTICLES_ACCELERATOR