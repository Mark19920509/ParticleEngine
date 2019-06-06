
#ifndef CLOTH_DEMO
#define CLOTH_DEMO

#include "../BaseDemo.h"
#include "ParticleEngine/PhysicsParticle.h"
#include <slmath/slmath.h>

class ClothDemo : public BaseDemo
{
public:
    virtual void Initialize();
    virtual void Release();

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

};


#endif // CLOTH_DEMO