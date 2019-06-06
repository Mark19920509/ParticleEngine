

#ifndef BASE_DEMO
#define BASE_DEMO

#include "ParticleEngine/PhysicsParticle.h"
#include <DX11Renderer.h>
#include <slmath/slmath.h>

class DX11Renderer;

class BaseDemo
{
public:
    virtual void Initialize() = 0;
    virtual void Simulate();
    virtual void IputKey(unsigned int wParam);
    virtual void Release();

    virtual slmath::vec4 *GetParticlePositions() const;
    virtual int GetParticlesCount() const;

    // Renderer
    
    virtual void InitializeRenderer(DX11Renderer &renderer);
    virtual void Draw(DX11Renderer &renderer) const;
    void InitializeOpenClData();

    static float Compress(const slmath::vec4 &vector);
    static slmath::vec4 UnCompress(float value);

    


protected:
    virtual void InitializeGraphicsObjectToRender(DX11Renderer &renderer);
    virtual bool IsUsingInteroperability() const;
    virtual void InitializeOpenCL(DX11Renderer &renderer);

protected:
    PhysicsParticle m_PhysicsParticle;
    static slmath::vec4 s_Colors [6];
    static const int s_ColorCount;

};



#endif // BASE_DEMO