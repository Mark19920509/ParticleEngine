
#include "ParticlesCollider.h"
#include "Utility/Timer.h"


void ParticlesCollider::AddInsideAabb(const Aabb& aabb)
{
    m_InsideAabb.push_back(aabb);
}

void ParticlesCollider::AddOutsideAabb(const Aabb& aabb)
{
    m_OutsideAabb.push_back(aabb);
}

void ParticlesCollider::AddInsideSphere(const Sphere& sphere)
{
    m_InsideSphere.push_back(sphere);
}

void ParticlesCollider::AddOutsideSphere(const Sphere& sphere)
{
    m_OutsideSphere.push_back(sphere);
}


Aabb *ParticlesCollider::GetInsideAabbs()
{
    if (m_InsideAabb.size() == 0)
    {
        return NULL;
    }
    return &m_InsideAabb[0];
}

Sphere *ParticlesCollider::GetOutsideSpheres()
{
    if (m_OutsideSphere.size() == 0)
    {
        return NULL;
    }
    return &m_OutsideSphere[0];
}

int ParticlesCollider::GetInsideAabbsCount() const
{
    return m_InsideAabb.size();
}

int ParticlesCollider::GetOutsideSpheresCount() const
{
    return m_OutsideSphere.size();
}

void ParticlesCollider::SatisfyCollisions(slmath::vec4 *particles, int particlesCount)
{
    Timer::GetInstance()->StartTimerProfile();
    for (int i = 0; i < particlesCount; i++)
    {
        slmath::vec4 &position = particles[i];
        SatisfyInsideAabb(position);
        SatisfyOutsideAabb(position);
        SatisfyInsideSphere(position);
        SatisfyOutsideSphere(position);
    }
    Timer::GetInstance()->StopTimerProfile("Shape Collision");
}


void ParticlesCollider::SatisfyInsideAabb(slmath::vec4 &position)
{
    const int aabbsCount = m_InsideAabb.size();
    for (int i = 0; i < aabbsCount; i++)
    {
        const Aabb& aabb = m_InsideAabb[i];
        position = slmath::clamp(position, aabb.m_Min, aabb.m_Max);
    }
}

void ParticlesCollider::SatisfyOutsideAabb(slmath::vec4 &/*position*/)
{
   
}

void ParticlesCollider::SatisfyInsideSphere(slmath::vec4 &position)
{
    const int spheresCount = m_InsideSphere.size();
    for (int i = 0; i < spheresCount; i++)
    {
        
        const Sphere& sphere = m_InsideSphere[i];
        const slmath::vec4 spherePosition = slmath::vec4(sphere.m_Position);
        const slmath::vec4 difference = position - spherePosition;
        const float sqrDistance = slmath::dot(difference, difference);
        if (sqrDistance > sphere.m_Radius * sphere.m_Radius)
        {
            position = spherePosition + (difference / sqrt(sqrDistance)) * sphere.m_Radius;
        }
    }
}

void ParticlesCollider::SatisfyOutsideSphere(slmath::vec4 &position)
{
    const int spheresCount = m_OutsideSphere.size();
    for (int i = 0; i < spheresCount; i++)
    {
        const Sphere& sphere = m_OutsideSphere[i];
        const slmath::vec4 spherePosition = slmath::vec4(sphere.m_Position);
        const slmath::vec4 difference = position - spherePosition;
        const float sqrDistance = slmath::dot(difference, difference);
        if (sqrDistance < sphere.m_Radius * sphere.m_Radius)
        {
            position = spherePosition + (difference / sqrt(sqrDistance)) * sphere.m_Radius;
        }
    }
}

void ParticlesCollider::Release()
{
    m_InsideAabb.clear();
    m_OutsideAabb.clear();
    m_InsideSphere.clear();
    m_OutsideSphere.clear();
}