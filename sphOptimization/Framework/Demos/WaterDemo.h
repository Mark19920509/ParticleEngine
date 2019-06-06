
#ifndef WATER_DEMO
#define WATER_DEMO

#include "../BaseDemo.h"
#include "ParticleEngine/PhysicsParticle.h"

#include <slmath/slmath.h>

class WaterDemo : public BaseDemo
{
public:
    virtual void Initialize();
    virtual void Release();

    virtual void Simulate();
    virtual void IputKey(unsigned int wParam);
    
    virtual void Draw(DX11Renderer &renderer) const;
    


protected:
    void InitializeGraphicsObjectToRender(DX11Renderer &renderer);
    virtual bool IsUsingInteroperability() const;

private:
    slmath::vec4 *m_Spheres;
    int m_SpheresCount;
};


#endif // WATER_DEMO