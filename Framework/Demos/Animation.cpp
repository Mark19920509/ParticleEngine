#include "Animation.h"

#include <slmath/slmath.h>
#include <stdlib.h>
#include <string>

namespace
{
    class Emiter
    {
        public:
            Emiter();
            void Initialize(vrVec4 *particlesPosition, int pariclesCount);

            void Update();

        private:
            int m_ActiveParticlesCount;
            int m_ActiveParticlesIncrement;
            float m_TimeDurantion;
            vrVec4 *m_ParticlesPosition;
            int m_ParticlesCount;
            slmath::vec3 m_AreaCreation;

            slmath::vec3 *m_Velocties;
            slmath::vec3 *m_Accelerations;

    };

    Emiter::Emiter():   m_ActiveParticlesCount(0),
                        m_ActiveParticlesIncrement(100),
                        m_TimeDurantion(0.0f),
                        m_ParticlesPosition(NULL),
                        m_ParticlesCount(0),
                        m_AreaCreation(10000.0f, 0.0f, 1000.0f)
    {
    }

    void Emiter::Initialize(vrVec4 *particlesPosition, int pariclesCount)
    {
        m_ParticlesPosition = particlesPosition;
        m_ParticlesCount    = pariclesCount;
        for (int i = 0; i < m_ParticlesCount; i++)
        {
            vrVec4 &position = m_ParticlesPosition[i];
            position.w = 0.0f;
        }
    }

    void Emiter::Update()
    {
        // Create new particles
        for (int i = m_ActiveParticlesCount; i < m_ActiveParticlesCount + m_ActiveParticlesIncrement; i++)
        {
            vrVec4 &position = m_ParticlesPosition[i];
            position.w = 0.002f;

            float sin, cos;
            position.x = float(rand() % int(m_AreaCreation.x) - int(m_AreaCreation.x) / 2) * 0.01f;
            slmath::sincos(position.x + m_TimeDurantion, &sin, &cos);
            position.y = sin;
            position.z = 0.0f; // float(rand() % int(m_AreaCreation.z) - int(m_AreaCreation.z) / 2) * 0.01f;
        }

        for (int i = 0; i < m_ParticlesCount; i++)
        {
            vrVec4 &position = m_ParticlesPosition[i];
            position.y += 0.1f;
        }

        m_ActiveParticlesCount += m_ActiveParticlesIncrement;

        if (m_ActiveParticlesCount + m_ActiveParticlesIncrement >= m_ParticlesCount)
        {
            for (int i = m_ActiveParticlesCount; i < m_ParticlesCount; i++)
            {
                vrVec4 &position = m_ParticlesPosition[i];
                position.w = 0.0f;
            }
            m_ActiveParticlesCount = 0;
        }

        m_TimeDurantion += 1.0f / 60.0f;
    }


    class Transformer
    {
        public:
            Transformer();
            void Initialize(const slmath::vec4 *startPositions, 
                            const slmath::vec4 *endPositions, 
                            slmath::vec4 *positions, 
                            int           positionCount);

            void Initialize(const slmath::vec4 *startPositions, 
                            const slmath::vec4 *endPositions,
                            float currentStep = 0.0f);

            void Update(float step, float delay = 0.0f);
            float GetCurrentStep() const;
            void SetCurrentStep(const float currentStep);

        private:
            const slmath::vec4 *m_StartPositions;
            const slmath::vec4 *m_EndPositions;
            slmath::vec4 *m_Positions;
            int           m_ParticlesCount;
            float         m_CurrentStep;
    };

    Transformer::Transformer() :    m_StartPositions(NULL), 
                                    m_EndPositions(NULL), 
                                    m_Positions(NULL), 
                                    m_ParticlesCount(0),
                                    m_CurrentStep(0)
    {
    }

    void Transformer::Initialize(const slmath::vec4 *startPositions, 
                                 const slmath::vec4 *endPositions, 
                                 slmath::vec4 *positions, 
                                 int           particlesCount)
    {
        m_StartPositions    = startPositions;
        m_EndPositions      = endPositions;
        m_Positions         = positions;
        m_ParticlesCount    = particlesCount;
        m_CurrentStep       = 0.0f;
    }

    void Transformer::Initialize(const slmath::vec4 *startPositions, const slmath::vec4 *endPositions, float currentStep /* = 0.0f */)
    {
        assert(m_Positions != NULL);
        m_StartPositions    = startPositions;
        m_EndPositions      = endPositions;
        m_CurrentStep       = currentStep;
    }


    void Transformer::Update(float step, float delay)
    {
        m_CurrentStep += step;
        float currentStep = slmath::saturate(m_CurrentStep);
        
        for (int i = 0; i < m_ParticlesCount; i++)
        {
            float iCurrentStep = m_CurrentStep + (float(i) / float(m_ParticlesCount)) * delay;
            iCurrentStep = slmath::saturate(iCurrentStep);

            slmath::vec4 startColor = BaseDemo::UnCompress(m_StartPositions[i].w);
            slmath::vec4 endColor   = BaseDemo::UnCompress(m_EndPositions[i].w);
            slmath::vec4 currentColor = startColor * (1.0f - currentStep) + endColor * currentStep;

            slmath::vec4 startPosition = m_StartPositions[i];
            startPosition.w = 0.0f;

            slmath::vec4 endPosition = m_EndPositions[i];
            endPosition.w = 0.0f;

            m_Positions[i] = startPosition * (1.0f - iCurrentStep) + endPosition * iCurrentStep;


            m_Positions[i].w = BaseDemo::Compress(currentColor);
        }        
    }

    float Transformer::GetCurrentStep()const
    {
        return m_CurrentStep;
    }

    void Transformer::SetCurrentStep(const float currentStep)
    {
        m_CurrentStep = currentStep;
    }

    Emiter      m_Emiter;
    Transformer m_Transformer;
}


void Animation::Initialize()
{

    m_AmimationTime = 0.0f;
    m_Transition    = 0;

    const float spaceBetweenParticles = 1.0f;
#ifdef DEBUG
    const int sideBox = 8;
#else
    const int sideBox = 32;
#endif //DEBUG
    const int halfSideBox = sideBox / 2;
    const int particlesCount = sideBox * sideBox * sideBox;
    m_ParticlesCount = particlesCount;
    
    // Allocate shapes
    m_StartPositions            = new slmath::vec4[particlesCount];
    m_ParametricSphere          = new slmath::vec4[particlesCount];
    m_ParametricCone            = new slmath::vec4[particlesCount];
    m_ParametricTorus           = new slmath::vec4[particlesCount];
    m_ParametricCylinder        = new slmath::vec4[particlesCount];
    m_ParametricTear            = new slmath::vec4[particlesCount];
    m_ParametricCrossCap        = new slmath::vec4[particlesCount];
    m_ParametricSteinersRoman   = new slmath::vec4[particlesCount];
    m_ParametricSeaShell        = new slmath::vec4[particlesCount];
    m_ParametricDini            = new slmath::vec4[particlesCount];
    m_ParametricHeart           = new slmath::vec4[particlesCount];



    m_ShapesCount  = 11;
    m_CurrentShape = 1;
    m_Shapes                    = new slmath::vec4*[m_ShapesCount];

    // Initialize shapes
    m_Shapes[0] = m_StartPositions         ;
    m_Shapes[1] = m_ParametricSphere       ;
    m_Shapes[2] = m_ParametricCone         ;
    m_Shapes[3] = m_ParametricTorus        ;
    m_Shapes[4] = m_ParametricCylinder     ;
    m_Shapes[5] = m_ParametricTear         ;
    m_Shapes[6] = m_ParametricCrossCap     ;
    m_Shapes[7] = m_ParametricSteinersRoman;
    m_Shapes[8] = m_ParametricSeaShell     ;
    m_Shapes[9] = m_ParametricDini         ;
    m_Shapes[10] = m_ParametricHeart       ;

    int index = 0;
    for (int i = 0; i < sideBox; i++)
    {
        for (int j = 0; j < sideBox; j++)
        {
            for (int k = 0; k < sideBox; k++)
            {
                assert(index < particlesCount );
                m_StartPositions[index] = slmath::vec4( (i - halfSideBox) * spaceBetweenParticles + float (rand() % 10) * 3.3f , 
                                                        (j - halfSideBox) * spaceBetweenParticles + float (rand() % 10) * 3.3f, 
                                                        (k - halfSideBox) * spaceBetweenParticles + float (rand() % 10) * 3.3f);

               // float floatIndex = float (index);
               //  m_StartPositions[index] = slmath::vec4( 10.0f * cos(5.0f * floatIndex / particlesCount), floatIndex / 20.0f, 10.0f * floatIndex * sin( 5.0f * floatIndex / particlesCount));

                m_ParametricSphere          [index] = m_StartPositions[index];
                m_ParametricCone            [index] = m_StartPositions[index];
                m_ParametricTorus           [index] = m_StartPositions[index];
                m_ParametricCylinder        [index] = m_StartPositions[index];
                m_ParametricTear            [index] = m_StartPositions[index];
                m_ParametricCrossCap        [index] = m_StartPositions[index];
                m_ParametricSteinersRoman   [index] = m_StartPositions[index];
                m_ParametricSeaShell        [index] = m_StartPositions[index];
                m_ParametricDini            [index] = m_StartPositions[index];
                m_ParametricHeart           [index] = m_StartPositions[index];
                index++;
            }
        }
    }

    int newSize = particlesCount;
    BaseDemo::FillPositionFromXml(std::string("people"), std::string("noi:GEO_POINT"), reinterpret_cast<slmath::vec4*>(m_StartPositions), newSize);
//    BaseDemo::FillPositionFromXml(std::string("chris"), std::string("vector"), reinterpret_cast<slmath::vec4*>(m_StartPositions), newSize);

    const float pi2 = 6.28318530f;
    const float pi = pi2 / 2.0f;

    // Sphere
    index = 0;
    int sqrShapesCount = int(sqrt(float(particlesCount)));
    for (int i = 0; i < sqrShapesCount; i++)
    {
        for (int j = 0; j < sqrShapesCount; j++)
        {
            float radius = 10.0f;
	        float fi = pi * (float(i) / float(sqrShapesCount));
            float theta = pi2 * (float(j) / float(sqrShapesCount));
            float sinFi,cosFi,sinTheta,cosTheta;
	        slmath::sincos( fi, &sinFi, &cosFi);
	        slmath::sincos( theta, &sinTheta, &cosTheta);
            slmath::vec3 normal = slmath::vec3(sinFi*cosTheta, sinFi*sinTheta, cosFi);
	        m_ParametricSphere[index++] = normal * radius;
        }
    }

    
    // Cone
    index = 0;
    for (int i = 0; i < sqrShapesCount; i++)
    {
        for (int j = 0; j < sqrShapesCount; j++)
        {
            float R = 10.0f;
            float H = 15.0f;
            float S = (float(j) / float(sqrShapesCount));
            float T = (float(i) / float(sqrShapesCount));
            float theta = pi2*T;
            float sinTheta,cosTheta;
	        slmath::sincos( theta, &sinTheta, &cosTheta);
	        m_ParametricCone[index++] = slmath::vec3(((H-S*H)/H)*R*cosTheta, ((H-S*H)/H)*R*sinTheta, S*H);
        }
    }

    // Torus
    index = 0;
    for (int i = 0; i < sqrShapesCount; i++)
    {
        for (int j = 0; j < sqrShapesCount; j++)
        {
            float pi2 = 6.28318530f;
            float M = 10.0f;
            float N = 5.0f;
	        float cosS, sinS;
	        slmath::sincos( pi2 * (float(i) / float(sqrShapesCount)), &sinS, &cosS);
	        float cosT, sinT;
	        slmath::sincos( pi2 * (float(j) / float(sqrShapesCount)), &sinT, &cosT);
	        m_ParametricTorus[index++] = slmath::vec3((M + N * cosT) * cosS, (M + N * cosT) * sinS, N * sinT);
        }
    }

    // Cylinder
    index = 0;
    for (int i = 0; i < sqrShapesCount; i++)
    {
        for (int j = 0; j < sqrShapesCount; j++)
        {
            float R = 5.0f;
            float H = 15.0f;
            float S = (float(j) / float(sqrShapesCount));
            float T = (float(i) / float(sqrShapesCount));
            float theta = pi2*T;
            float sinTheta,cosTheta;
	        slmath::sincos( theta, &sinTheta, &cosTheta);
	        m_ParametricCylinder[index++] = slmath::vec3(((H-S*H)/H)*R*cosTheta, ((H-S*H)/H)*R*sinTheta, S*H);
        }
    }

    // Tear
    index = 0;
    for (int i = 0; i < sqrShapesCount; i++)
    {
        for (int j = 0; j < sqrShapesCount; j++)
        {
            float S = (float(i) / float(sqrShapesCount));
            float T = (float(j) / float(sqrShapesCount));
            float fi = pi2*S;
            float theta = pi*T;
            float sinFi, cosFi, sinTheta, cosTheta;
	        slmath::sincos( fi, &sinFi, &cosFi);
	        slmath::sincos( theta, &sinTheta, &cosTheta);
	        m_ParametricTear[index++] = slmath::vec3( 10.0f * 0.5f*(1 - cosTheta)*sinTheta*cosFi,
								                      10.0f * 0.5f*(1 - cosTheta)*sinTheta*sinFi,
								                      10.0f * cosTheta);
        }
    }

    // Cross cap
    index = 0;
    for (int i = 0; i < sqrShapesCount; i++)
    {
        for (int j = 0; j < sqrShapesCount; j++)
        {
	        float u = (float(i) / float(sqrShapesCount)) * pi;
	        float v = (float(j) / float(sqrShapesCount)) * pi;
	        float sinu,cosu,sinv,cosv,sin2v;
	        slmath::sincos( u, &sinu, &cosu );
	        slmath::sincos( v, &sinv, &cosv );
	        sin2v = sin( 2*v );

	        m_ParametricCrossCap[index++] = slmath::vec3(10.0f * cosu*sin2v, 10.0f * sinu*sin2v, 10.0f * cosv*cosv - cosu*cosu*sinv*sinv);
	
        }
    }


    // Steiners Roman
    index = 0;
    for (int i = 0; i < sqrShapesCount; i++)
    {
        for (int j = 0; j < sqrShapesCount; j++)
        {
	        float u = (float(i) / float(sqrShapesCount)) * pi;
	        float v = (float(j) / float(sqrShapesCount))*pi;
	        float sinu,cosu,sinv,cosv,sin2v,sin2u;
	        slmath::sincos( u, &sinu, &cosu );
	        slmath::sincos( v, &sinv, &cosv );
	        sin2v = sin( 2*v );
	        sin2u = sin( 2*u );
	        float a = 3.0f;	
	        m_ParametricSteinersRoman[index++] = slmath::vec3(a*a*cosv*cosv*sin2u/2, a*a*sinu*sin2v/2, a*a*cosu*sin2v/2);
        }
    }

    // Sea Shell
    index = 0;
    for (int i = 0; i < sqrShapesCount; i++)
    {
        for (int j = 0; j < sqrShapesCount; j++)
        {
	        float x,y,z;
	        float s = (float(i) / float(sqrShapesCount));
	        float t = (float(j) / float(sqrShapesCount));
	        x = 2.0f * (1.0f - pow(2.73f, (s * 6 * pi) / (6 * pi))) * cos(s * 6 * pi) * cos((t * 2 * pi) / 2) * cos((t * 2 * pi) / 2);
	        y = 2.0f * (-1.0f + pow(2.73f, (s * 6 * pi) / (6 * pi))) * sin(s * 6 * pi) * cos((t * 2 * pi) / 2) * cos((t * 2 * pi) / 2);
	        z = 1.0f - pow(2.73f, (s * 6 *pi) / (3 * pi)) - sin(t * 2 * pi) + pow(2.73f, (s * 6 * pi) / (6 * pi)) * sin(t * 2 *pi);
	        m_ParametricSeaShell[index++] = slmath::vec3(x, y, z) * 10.0f;
        }
    }

    // Dini
    index = 0;
    for (int i = 0; i < sqrShapesCount; i++)
    {
        for (int j = 0; j < sqrShapesCount; j++)
        {
	        float x,y,z;
	        float s = (float(i) / float(sqrShapesCount));
	        float t = (float(j) / float(sqrShapesCount));
	        x = 10.0f * cos(s * 4 * pi) * sin(t + 0.01f);
	        y = 10.0f * sin(s * 4 * pi) * sin(t + 0.01f);
	        z = 1.0f * (cos(t + 0.01f) + log(tan((t + 0.01f) / 2))) + 0.2f * s * 4 * pi;
	        m_ParametricDini[index++] = slmath::vec3(x, y, z);
        }
    }


    // Heart
    index = 0;
    for (int i = 0; i < sqrShapesCount; i++)
    {
        for (int j = 0; j < sqrShapesCount; j++)
        {
	        float x,y,z;
	        float s = (float(i) / float(sqrShapesCount));
	        float t = (float(j) / float(sqrShapesCount));
	        x = cos(s * pi) * sin(t * 2 * pi) - pow(abs(sin(s * pi) * sin(t * 2 * pi)), 0.5f) * 0.5f;
	        y = cos(t * 2 * pi) * 0.5f;
	        z = sin(s * pi) * sin(t * 2 * pi);
	        m_ParametricHeart[index++] = slmath::vec3(10.0f * x, 10.0f * y, 10.0f * z);
        }
    }

    
    // Setup color
    for (int i = 0; i < particlesCount; i++)
    {
        m_StartPositions            [i].w = Compress(slmath::vec4(0.1f, 0.2f, 0.1f, 0.4f));
        m_ParametricSphere          [i].w = Compress(slmath::vec4(0.1f, 0.2f, 0.1f, 0.4f));
        m_ParametricCone            [i].w = Compress(slmath::vec4(0.1f, 0.2f, 0.1f, 0.4f));
        m_ParametricTorus           [i].w = Compress(slmath::vec4(0.1f, 0.2f, 0.1f, 0.4f));
        m_ParametricCylinder        [i].w = Compress(slmath::vec4(0.1f, 0.2f, 0.1f, 0.4f));
        m_ParametricTear            [i].w = Compress(slmath::vec4(0.1f, 0.2f, 0.1f, 0.4f));
        m_ParametricCrossCap        [i].w = Compress(slmath::vec4(0.1f, 0.2f, 0.1f, 0.4f));
        m_ParametricSteinersRoman   [i].w = Compress(slmath::vec4(0.1f, 0.2f, 0.1f, 0.4f));
        m_ParametricSeaShell        [i].w = Compress(slmath::vec4(0.1f, 0.2f, 0.1f, 0.4f));
        m_ParametricDini            [i].w = Compress(slmath::vec4(0.1f, 0.2f, 0.1f, 0.4f));
        m_ParametricHeart           [i].w = Compress(slmath::vec4(0.1f, 0.2f, 0.1f, 0.4f));
    }


    m_PhysicsParticle.SetParticlesMass(10.0f);
    slmath::vec4 gravity(10.0f, 0.0f, 0.0f);
    m_PhysicsParticle.SetDamping(1.0f);
    m_PhysicsParticle.SetParticlesAcceleration(*reinterpret_cast<vrVec4*>(&gravity));

    m_PhysicsParticle.SetEnableAcceleratorOnGPU(false);
    m_PhysicsParticle.SetEnableGridOnGPU(false);
    m_PhysicsParticle.SetEnableSPHAndIntegrateOnGPU(false);
    m_PhysicsParticle.SetEnableCollisionOnGPU(false);
    m_PhysicsParticle.SetEnableAnimation(true);
    

    m_PhysicsParticle.SetIsUsingCPU(false);

    m_PhysicsParticle.SetAnimationTime(0.0f);
    m_PhysicsParticle.SetParticlesAnimation(reinterpret_cast<vrVec4*>(m_ParametricDini));
    
    // Initialize particles
     m_PhysicsParticle.Initialize(reinterpret_cast<vrVec4*>(m_StartPositions), particlesCount);

     m_ParticleToAdd = 0;

     // m_Emiter.Initialize(m_PhysicsParticle.GetParticlePositions() + (sideBox * sideBox), sideBox);

     slmath::vec4 * particles = reinterpret_cast<slmath::vec4*> (m_PhysicsParticle.GetParticlePositions());

     m_Transformer.Initialize(m_StartPositions, m_ParametricSphere, particles, sqrShapesCount * sqrShapesCount
     );
}

void Animation::Release()
{
    BaseDemo::Release();
    delete[] m_StartPositions;
    delete[] m_ParametricSphere;
    delete[] m_ParametricCone;
    delete[] m_ParametricTorus;
    delete[] m_ParametricCylinder;
    delete[] m_ParametricTear;
    delete[] m_ParametricCrossCap;
    delete[] m_ParametricSteinersRoman;
    delete[] m_ParametricSeaShell;
    delete[] m_ParametricDini;
    delete[] m_ParametricHeart;
}




void Animation::Simulate(float /*delatT*/)
{
    m_Transition;

    m_AmimationTime += 0.01f;

    if (m_Transition % 2 == 0 && m_AmimationTime >= 1.1f)
    {
        m_AmimationTime = 0.0f;
        m_PhysicsParticle.SetParticlesAnimation(reinterpret_cast<vrVec4*>(m_StartPositions));
        m_PhysicsParticle.InitializeOpenClData();

        if (m_CurrentShape >= m_ShapesCount) 
        {
            m_CurrentShape = 1;
        }
        m_Transition++;
    }
    else if (m_Transition % 2 == 1 && m_AmimationTime >= 1.2f)
    {
        m_AmimationTime = 0.0f;
        int nextStep = m_CurrentShape < m_ShapesCount - 1 ?  m_CurrentShape + 1 : 0;

        m_PhysicsParticle.SetParticlesAnimation(reinterpret_cast<vrVec4*>(m_Shapes[nextStep]));
        m_PhysicsParticle.InitializeOpenClData();

        m_CurrentShape++;

        if (m_CurrentShape >= m_ShapesCount) 
        {
            m_CurrentShape = 1;
        }
        m_Transition++;
    }

    m_PhysicsParticle.SetAnimationTime(slmath::min(m_AmimationTime,1.0f));
    m_PhysicsParticle.Simulate();

}

void Animation::IputKey(unsigned int /*wParam*/)
{
}


bool Animation::IsUsingInteroperability() const
{
    return true;
}