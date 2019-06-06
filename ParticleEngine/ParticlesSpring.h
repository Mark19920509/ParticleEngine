#ifndef PARTICLES_SPRING
#define PARTICLES_SPRING

#include <vector>


namespace slmath
{
    class vec4;
}

struct Spring
{
    int m_ParticleIndex1;
    int m_ParticleIndex2;
    float m_Distance;
    float m_Pad;
};

class ParticlesSpring
{
    
public:
    void AddSpring(const Spring& spring);
    void Solve(slmath::vec4 *positions, int positionsCount);
    void Release();

    const Spring* GetSprings() const;
    int GetSpringsCount() const;
    

private:
    std::vector<Spring> m_SpringsList;
};

#endif // PARTICLES_SPRING