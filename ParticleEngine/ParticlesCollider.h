
#ifndef PARTICLES_COLLIDER
#define PARTICLES_COLLIDER

#include <vector>
#include <slmath/slmath.h>

struct SLMATH_ALIGN16  Aabb
{
    slmath::vec4 m_Min;
    slmath::vec4 m_Max;

    Aabb() : m_Min(0.0f), m_Max(0.0f) {}
};

struct SLMATH_ALIGN16 Sphere
{
    slmath::vec3 m_Position;
    float m_Radius;
    Sphere() : m_Position(0.0f), m_Radius(0.0f) {}
};

class ParticlesCollider
{
public:

    void AddInsideAabb(const Aabb& aabb);
    void AddOutsideAabb(const Aabb& aabb);
    void AddInsideSphere(const Sphere& sphere);
    void AddOutsideSphere(const Sphere& sphere);

    void SatisfyCollisions(slmath::vec4 *particles, int particlesCount);

    void Release();

    Aabb    *GetInsideAabbs();
    Sphere  *GetInsideSpheres();
    Sphere  *GetOutsideSpheres();

    int GetInsideAabbsCount() const;
    int GetInsideSpheresCount() const;
    int GetOutsideSpheresCount() const;


private:

    void SatisfyInsideAabb(slmath::vec4 &position);
    void SatisfyOutsideAabb(slmath::vec4 &position);
    void SatisfyInsideSphere(slmath::vec4 &position);
    void SatisfyOutsideSphere(slmath::vec4 &position);


    std::vector<Aabb> m_InsideAabb;
    std::vector<Aabb> m_OutsideAabb;

    std::vector<Sphere> m_InsideSphere;
    std::vector<Sphere> m_OutsideSphere;

    Sphere  m_NullSphere;
    Aabb    m_NullAabb;
};

#endif // PARTICLES_COLLIDER