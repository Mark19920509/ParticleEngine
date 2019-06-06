#include "BaseDemo.h"
#include <slmath/slmath.h>

slmath::vec4 BaseDemo::s_Colors[] = {   slmath::vec4(1.0f, 0.1f, 0.1f, 0.45f),
                                        slmath::vec4(0.1f, 1.0f, 0.1f, 0.45f),
                                        slmath::vec4(0.1f, 0.1f, 1.0f, 0.45f),
                                        slmath::vec4(1.0f, 1.0f, 0.1f, 0.45f),
                                        slmath::vec4(1.0f, 0.1f, 1.0f, 0.45f),
                                        slmath::vec4(0.1f, 1.0f, 1.0f, 0.45f)
                                    };

const int BaseDemo::s_ColorCount = 6;

void BaseDemo::Simulate()
{
    m_PhysicsParticle.Simulate();
}

void BaseDemo::IputKey(unsigned int /*wParam*/)
{
}

void BaseDemo::Release()
{
    m_PhysicsParticle.Release();
}

slmath::vec4 *BaseDemo::GetParticlePositions() const
{
    return reinterpret_cast<slmath::vec4 *>(m_PhysicsParticle.GetParticlePositions());
}

int BaseDemo::GetParticlesCount() const
{
    return m_PhysicsParticle.GetParticlesCount();
}

bool BaseDemo::IsUsingInteroperability() const
{
    return false;
}

void BaseDemo::InitializeGraphicsObjectToRender(DX11Renderer &renderer)
{
    renderer.InitializeParticles(GetParticlesCount(), 0, 0);
}

// By default all physics particles but 0 mesh
void BaseDemo::InitializeRenderer(DX11Renderer &renderer)
{
    const bool isUsingInteroperability = IsUsingInteroperability();
    renderer.SetIsUsingInteroperability(isUsingInteroperability);
    m_PhysicsParticle.SetIsUsingInteroperability(isUsingInteroperability);

    InitializeGraphicsObjectToRender(renderer);

    InitializeOpenCL(renderer);
}

void BaseDemo::InitializeOpenCL(DX11Renderer &/*renderer*/)
{
    m_PhysicsParticle.InitializeOpenCL(/*renderer.GetID3D11Device()*/NULL, NULL/*renderer.GetID3D11Buffer()*/);
}

void BaseDemo::InitializeOpenClData()
{
    m_PhysicsParticle.InitializeOpenClData();
}

void BaseDemo::Draw(DX11Renderer &renderer) const
{
    renderer.Render(GetParticlePositions(), GetParticlesCount());
}


float BaseDemo::Compress(const slmath::vec4 &vector)
{
    // Supported compression
    assert(vector.x >= 0.0f && vector.x <= 1.0f);
    assert(vector.y >= 0.0f && vector.y <= 1.0f);
    assert(vector.z >= 0.0f && vector.z <= 1.0f);
    // Size support from 0.0f to 51.0f
    assert(vector.w >= 0.0f && vector.w <= 51.0f);

    slmath::vec4 toCompress = vector;
    unsigned int  v1 = unsigned int (toCompress.x * 0xFF);
    unsigned int  v2 = unsigned int(toCompress.y * 0xFF);
    unsigned int  v3 = unsigned int(toCompress.z * 0xFF);
    unsigned int  v4 = unsigned int (toCompress.w * 5.0f);
    
    v1 =  v1 > 255 ? 255 : v1;
    v2 =  v2 > 255 ? 255 : v2;
    v3 =  v3 > 255 ? 255 : v3;
    v4 =  v4 > 255 ? 255 : v4;
   

    unsigned int store = ((v1 & 0xFF) << 0) | ((v2 & 0xFF) << 8) | ((v3 & 0xFF) << 16) | ((v4 & 0xFF) << 24);
    float returnValue = *reinterpret_cast<float*>(&store);

    #ifdef DEBUG
        slmath::vec4 original = UnCompress(returnValue);
        const slmath::vec4 epsiolon(1.0f);
        assert(original <= toCompress + epsiolon && original >= toCompress - epsiolon );
    #endif // DEBUG

    return returnValue;
}

slmath::vec4 BaseDemo::UnCompress(float value)
{
    unsigned int uncompress = *reinterpret_cast<unsigned int*>(&value);

    unsigned int o1, o2, o3, o4;
    o1 = (unsigned int) uncompress & 0xFF;
    o2 = (unsigned int)(uncompress >> 8) & 0xFF;
    o3 = (unsigned int)(uncompress >> 16) & 0xFF;
    o4 = (unsigned int)(uncompress >> 24) & 0xFF;

    slmath::vec4 vector(float(o1) / 255.0f,
                        float(o2) / 255.0f,
                        float(o3) / 255.0f,
                        float(o4) / 5.0f);

    return vector;
}
