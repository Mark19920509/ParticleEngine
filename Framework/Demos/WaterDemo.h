
#ifndef WATER_DEMO
#define WATER_DEMO

#include "../BaseDemo.h"
#include "ParticleEngine/PhysicsParticle.h"

#include <slmath/slmath.h>

class WaterDemo : public BaseDemo
{
public:
    WaterDemo() : m_Spheres(0), m_SpheresCount(0), mReduction(0.0f), mColorIndex(0){}

    virtual void Initialize();
    virtual void Release();

    virtual void Simulate(float deltaT);
    virtual void IputKey(unsigned int wParam);
    
    virtual void Draw(DX11Renderer &renderer) const;
    


protected:
    virtual void InitializeGraphicsObjectToRender(DX11Renderer &renderer);
    virtual bool IsUsingInteroperability() const;

private:
	void ChangeColor();

    slmath::vec4 *m_Spheres;
    int m_SpheresCount;

	float mReduction;

    int m_Step;

	int mColorIndex;
};


#endif // WATER_DEMO