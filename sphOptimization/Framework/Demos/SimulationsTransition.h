
#ifndef SIMULATIONS_TRANSITION
#define SIMULATIONS_TRANSITION

#include "../BaseDemo.h"
#include "ParticleEngine/PhysicsParticle.h"
#include <slmath/slmath.h>

class SimulationsTransition : public BaseDemo
{
public:
    virtual void Initialize();
    virtual void Release();

    virtual void Simulate();

    virtual void InitializeGraphicsObjectToRender(DX11Renderer &renderer);
    virtual void Draw(DX11Renderer &renderer) const;

private:

    void CreateSpring(int index1, int index2);

#ifdef DEBUG
    const static int m_SideCloth;
    const static int m_ClothCount;
#else
    const static int m_SideCloth;
    const static int m_ClothCount;
#endif //DEBUG

    slmath::vec4 *m_StartPositions;
    slmath::vec4* m_Spheres;
    int m_SpheresCount;

    int m_FramesCount;

};


#endif // SIMULATIONS_TRANSITION