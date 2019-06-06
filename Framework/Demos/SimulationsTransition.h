
#ifndef SIMULATIONS_TRANSITION
#define SIMULATIONS_TRANSITION

#include "../BaseDemo.h"
#include "ParticleEngine/PhysicsParticle.h"
#include <slmath/slmath.h>
#include <process.h>

class Camera;

 // Multithreading
struct ThreadData
{
    int mutexIndex;
    PhysicsParticle *physicsParticle;
};

class SimulationsTransition : public BaseDemo
{
public:
    SimulationsTransition();
    virtual void Initialize();
    virtual void Release();

    virtual void Simulate(float delatT);

    virtual void InitializeRenderer(DX11Renderer &renderer);
    virtual void InitializeGraphicsObjectToRender(DX11Renderer &renderer);
    virtual void Draw(DX11Renderer &renderer) const;

    virtual void SetCamera(Camera *camera);


    enum State {
                eStart = 0, 
                eZoomIn, 
                eRotateAroundCloth,
                eMoveAway, 
                eRevolution, 
                eGoToForceField,
                eMoveSpheres, 
                eStrongWind, 
                eClothFlying,
                eParticleFollowCamera,
                eForceField,
                e2ForceField,
                eStopMotion,
                eCollison,
                eAttraction,
                eAnimationForceField2,
                eAnimationWater1,
                eWater1,
                eAnimationWater2,
                eWater2,
                eAnimationWater3,
                eWater3,
                eAnimationWater4,
                eWater4,
                eStateCount
                };

    struct Event
    {
        State m_State;
        float m_TimeDuration;
        float m_TimeAccumulation;
    };

    //struct Action
    //{
    //    Event* m_Event;
    //    float  m_TimeAccumulation;
    //    void   AccumaleTime(Event* /*Events*/) {}
    //    void operator()() { }

    //};

private:

    void CheckActionStates();
    void InitActions();
    // void SimulateOnCPU();

    void DetachClothes();
    void DetachCloth(int index);
    void CreateCloth();
    void CreateSpring(int index1, int index2);
    void CreateCollisionsForCloth();

    void StartWind();
    void StrongtWind();

    void Decelerate();
    
    void MoveSpheres();
    void GoToForceField();
    void GoToCenter();

    void TurnCamera(float angle, bool withLimit = true, float angularSpeed = 0.2f);
    void ZoomIn();
    void MoveIntoCloth();
    void MoveSphereForCamera();
    void RotateCamera();
    void RotateCamera2();

    void CreateForceField();
    void ForceFieldColor(int index);
    void StartForceField();
    void MoveForceField();
    void KillSpeedForceField();
    void AttractForceField();
    void MoveCameraToAttraction();
    void InitAnimationForceField2();
    void RepulseForceField();
    void StartForceFieldCollision();
    void CreateCollisionsForForceField();

    void CreateSky();
    void InitAnimationSky3();

    void CreateCollisionsForFlyingParticles();
    void CreateFlyingParticles();
    void MoveAccelerator4();
    void MoveAcceleratorDirection4();
    void RevoluteAccelerator4();
    void FollowParticles();
    void MoveClothAcceleratorByParticles4();
    void MoveLight();
    void MoveSphere4();
    void MoveFlyingParticlesByForceField();

    void CreateCollisionsForWater();
    void CreateWater();
    void InitAnimationWater(size_t index);
    void InitWater();
    void KillSpeedWater();
    void MoveCameraToWater();


    
    const static int m_ParticlesClolorCount = 10;
    int pad[1]; // To be 128 bits aligned
    slmath::vec4    m_ParticlesColors[m_ParticlesClolorCount];

    const static int m_ParticlesClolor3Count = 10;
    slmath::vec4    m_ParticlesColors3[m_ParticlesClolor3Count];

    const static int m_ParticlesClolor4Count = 10;
    slmath::vec4    m_ParticlesColors4[m_ParticlesClolor4Count];

   

    ThreadData* m_ThreadData;
    int         m_CpuCount;
    HANDLE*     m_RunMutex;

    // Data
    
    int m_SideCloth;
    int m_ClothCount;
    float m_SpaceBetweenCloth;

    int m_ClothActivatedCount;
    int m_RenderSpheresCount;

    
    Event* m_Events;
    Event* m_CurrentEvent;
    
    
    slmath::vec4 *m_StartPositions;
    int m_ParticlesCount;

    slmath::vec4 *m_StartPositions2;
    int m_Particles2Count;

    slmath::vec4 *m_StartPositions3;
    int m_Particles3Count;
    
    slmath::vec4 *m_StartPositions4;
    int m_Particles4Count;

    slmath::vec4 *m_StartPositions5;
    int m_Particles5Count;
    
    slmath::vec4* m_Spheres;
    int m_SpheresCount;

    slmath::vec4 *m_ParticleAnimation[5];

    int m_Particles2ForClothLines;

    slmath::vec3 m_InitialFlyingPosition;
    

    int m_FramesCount;
    float m_Time;
    float m_TimeAcumulator;
    float m_DeltaT;
    float m_LectureSpeed;
    float m_CameraDesiredAngle;
    slmath::vec3 m_ParticleVelocity;


    Camera *m_Camera;

    vrAccelerator   m_Accelerator;
    vrAccelerator   m_AcceleratorForceField;
    vrAccelerator   m_AcceleratorForceField3;
    vrAccelerator   m_AcceleratorForceField4;
    vrAccelerator   m_AcceleratorWater5;
    
    slmath::vec3    m_VelocityAcceleratorForceField;
    float           m_SpeedAccelerator;
    slmath::vec3    m_VelocityAccelerator;

    int *m_Indirections;
    int m_DetachedClothCount;
    

    PhysicsParticle m_PhysicsParticle2;
    PhysicsParticle m_PhysicsParticle3;
    PhysicsParticle m_PhysicsParticle4;
    PhysicsParticle m_PhysicsParticle5;
    size_t m_RendererIndex3;
    size_t m_RendererIndex4;
    size_t m_RendererIndex5;

};


#endif // SIMULATIONS_TRANSITION