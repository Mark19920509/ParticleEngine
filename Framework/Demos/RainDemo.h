
#ifndef RAIN_DEMO
#define RAIN_DEMO

#include "../BaseDemo.h"
#include "ParticleEngine/PhysicsParticle.h"

#include <slmath/slmath.h>

class RainDemo : public BaseDemo
{
public:
    virtual void Initialize();
    virtual void Release();

    virtual void Draw(DX11Renderer &renderer) const;

protected:
    virtual void InitializeGraphicsObjectToRender(DX11Renderer &renderer);
    virtual bool IsUsingInteroperability() const;

private:
    slmath::vec4 *m_Spheres;
    vrAccelerator m_Accelerator;
};


#endif // RAIN_DEMO