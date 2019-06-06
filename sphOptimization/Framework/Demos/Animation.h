
#ifndef ANIMATION
#define ANIMATION

#include "../BaseDemo.h"
#include "ParticleEngine/PhysicsParticle.h"

class Animation : public BaseDemo
{
public:
    virtual void Initialize();
    virtual void Release();
    virtual void Simulate();
    virtual void IputKey(unsigned int wParam);

private:
    int m_ParticleToAdd;
    slmath::vec4 *m_StartPositions;
    slmath::vec4 *m_ParametricSphere;
    slmath::vec4 *m_ParametricCone;
    slmath::vec4 *m_ParametricTorus;
    slmath::vec4 *m_ParametricCylinder;
    slmath::vec4 *m_ParametricTear;
    slmath::vec4 *m_ParametricCrossCap;
    slmath::vec4 *m_ParametricSteinersRoman;
    slmath::vec4 *m_ParametricSeaShell;
    slmath::vec4 *m_ParametricDini;
    slmath::vec4 *m_ParametricHeart;

    int m_ShapesCount;
    slmath::vec4 **m_Shapes;
    int m_CurrentShape;

};


#endif // ANIMATION