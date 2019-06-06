
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

    virtual void Simulate(float delatT /*=  1 / 60.0f */);
    virtual void InitializeGraphicsObjectToRender(DX11Renderer &renderer);
    virtual void Draw(DX11Renderer &renderer) const;
    virtual void IputKey(unsigned int wParam);
    virtual bool IsUsingInteroperability() const;

private:

    void CreateCloth(PhysicsParticle *physicsParticle, const slmath::vec3& globalPosition, int clothCount, bool isUsingCPU);
    void CreateSpring(PhysicsParticle *physicsParticle, int index1, int index2, slmath::vec4 *startPositions);


    int m_SideCloth;
    int m_ClothCount1;
    int m_ClothCount2;


    slmath::vec4* m_Spheres;
    vrAccelerator m_Accelerator;

    PhysicsParticle m_PhysicsParticle2;

};


#endif // CLOTH_DEMO