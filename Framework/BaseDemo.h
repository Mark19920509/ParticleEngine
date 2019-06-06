

#ifndef BASE_DEMO
#define BASE_DEMO

#include "ParticleEngine/PhysicsParticle.h"
#include <DX11Renderer.h>
#include <slmath/slmath.h>
#include <string>

class DX11Renderer;
class Camera;


class BaseDemo
{
public:
    BaseDemo();

    virtual void Initialize() = 0;
    virtual void Simulate(float delatT = 1 / 60.0f);
    virtual void IputKey(unsigned int wParam);
    virtual void Release();

    virtual slmath::vec4 *GetParticlePositions() const;
    virtual int GetParticlesCount() const;

    const slmath::vec3& GetLightPosition() const;

    // Renderer
    
    virtual void InitializeRenderer(DX11Renderer &renderer);
    virtual void Draw(DX11Renderer &renderer) const;
    void InitializeOpenClData();

    // Camera
    virtual void SetCamera(Camera *) {}

    static float Compress(const slmath::vec4 &vector);
    static slmath::vec4 UnCompress(float value);
    static int FillPositionFromXml(const std::string& fileName, const std::string& tag, slmath::vec4* positions, int positionsCount);
    static void ConvertFiles();


protected:
    virtual void InitializeGraphicsObjectToRender(DX11Renderer &renderer);
    virtual bool IsUsingInteroperability() const;
    virtual void InitializeOpenCL(DX11Renderer &renderer);

protected:
    PhysicsParticle m_PhysicsParticle;
    static slmath::vec4 s_Colors [6];
    static const int s_ColorCount;
    
    slmath::vec3 m_LightPosition;

};



#endif // BASE_DEMO