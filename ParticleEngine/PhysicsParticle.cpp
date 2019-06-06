
#include "PhysicsParticle.h"

#include <slmath/slmath.h>

#include "ParticlesCollider.h"
#include "SmoothedParticleHydrodynamics.h"
#include "VerletIntegration.h"
#include "Grid3D.h"
#include "ParticlesSpring.h"
#include "ParticlesAccelerator.h"
#include "PipelineDescription.h"
#include "ParticlesGPU/ParticlesGPU.hpp"


#include "Utility/Timer.h"




struct PhysicsParticle::Pimpl
{
    Grid3D m_Grid3D;
    SmoothedParticleHydrodynamics   m_SmoothedParticleHydrodynamics;
    ParticlesCollider               m_ParticlesCollider;
    VerletIntegration               m_VerletIntegration;
    ParticlesSpring                 m_ParticlesSpring;
    ParticlesAccelerator            m_ParticlesAccelerator;
    ParticlesGPU                    m_ParticlesGPU;
    slmath::vec4*                   m_EndsAnimation;

    PipelineDescription             m_Pipeline;

    Pimpl() : m_SmoothedParticleHydrodynamics(&m_Grid3D)
            , m_ParticlesGPU()
            , m_EndsAnimation(NULL)
    {
    }
};


PhysicsParticle::PhysicsParticle() :    // Private parameters disabled
                                        m_IsUsingGrid3D         (false),
                                        m_SPHSimulation         (false),
                                        m_ContinuousIntegration (false),
                                        m_IsUsingAccelerator    (false),
                                        m_IsIntegrating         (false),
                                        m_IsSolvingSpring       (false),
                                        m_IsColliding           (false),
                                        m_Pimpl(new Pimpl)
{
}

PhysicsParticle::~PhysicsParticle()
{
    if (m_Pimpl->m_ParticlesGPU.cleanup() != 0)
    {
        assert(0);
    }
    delete m_Pimpl;
}

vrVec4 *PhysicsParticle::GetParticlePositions() const
{
    return reinterpret_cast<vrVec4*>(m_Pimpl->m_VerletIntegration.GetParticlePositions());
}

vrVec4 *PhysicsParticle::GetPreviousPositions() const
{
    return reinterpret_cast<vrVec4*>(m_Pimpl->m_VerletIntegration.GetParticlePreviousPositions());
}

int PhysicsParticle::GetParticlesCount() const
{
    return m_Pimpl->m_VerletIntegration.GetParticlesCount();
}

// Properies Setters
void PhysicsParticle::SetParticlesMass(float mass)
{
    m_Pimpl->m_SmoothedParticleHydrodynamics.SetParticlesMass(mass);
}

void PhysicsParticle::SetParticlesGazConstant(float gazConstant)
{
    m_Pimpl->m_SmoothedParticleHydrodynamics.SetParticlesGazConstant(gazConstant);
}

void PhysicsParticle::SetParticlesViscosity(float viscosity)
{
    m_Pimpl->m_SmoothedParticleHydrodynamics.SetParticlesMuViscosityt(viscosity);
}

void PhysicsParticle::SetParticlesAcceleration(const vrVec4 &acceleration)
{
    m_Pimpl->m_VerletIntegration.SetCommonAcceleration(*reinterpret_cast<const slmath::vec4*>(&acceleration));
}

void PhysicsParticle::SetParticlesAnimation(vrVec4 *endsAnimation)
{
    m_Pimpl->m_EndsAnimation = reinterpret_cast<slmath::vec4*>(endsAnimation);
}

void PhysicsParticle::SetDamping(float damping)
{
    m_Pimpl->m_VerletIntegration.SetDamping(damping);
}

// Properies Getters
float PhysicsParticle::GetParticlesMass() const
{
    return m_Pimpl->m_SmoothedParticleHydrodynamics.GetParameters().m_Mass;
}

float PhysicsParticle::GetParticlesGazConstant() const
{
    return m_Pimpl->m_SmoothedParticleHydrodynamics.GetParameters().m_GazConstant;
}

float PhysicsParticle::GetParticlesViscosity() const
{
    return m_Pimpl->m_SmoothedParticleHydrodynamics.GetParameters().m_MuViscosity;
}


void PhysicsParticle::CreateGrid()
{
     m_Pimpl->m_Grid3D.Initialize(   m_Pimpl->m_VerletIntegration.GetParticlePositions(), 
                            m_Pimpl->m_VerletIntegration.GetParticlesCount());
}

void PhysicsParticle::CreateGrid(vrVec4 *positions, int positionsCount)
{
    m_Pimpl->m_Grid3D.Initialize(reinterpret_cast<slmath::vec4*>(positions), positionsCount);
}

int PhysicsParticle::GetNeighbors(int currentIndex,  int *neighbors, int neighborsMaxCount)
{
    return m_Pimpl->m_Grid3D.GetNeighbors(currentIndex, neighbors, neighborsMaxCount);
}

void PhysicsParticle::Initialize(vrVec4 *positions, int positionsCount)
{
    Timer::GetInstance()->StartTimerProfile();
    
    // Initialze grid to allocate memory
    if (m_Pimpl->m_Pipeline.m_IsCreatingGridOnGPU)
    {
        CreateGrid(positions, positionsCount);
    }

    Timer::GetInstance()->StartTimerProfile();
    m_Pimpl->m_VerletIntegration.Initialize(reinterpret_cast<slmath::vec4*>(positions), positionsCount);
    if (m_Pimpl->m_Pipeline.m_IsCreatingGridOnGPU)
    {
        m_Pimpl->m_VerletIntegration.SetGrid3D(&m_Pimpl->m_Grid3D);
    }
    //m_Pimpl->m_ParticlesAccelerator.AllocateAccelerations(positionsCount);
    Timer::GetInstance()->StopTimerProfile("Init Verlet");
    Timer::GetInstance()->StartTimerProfile();

    if (m_Pimpl->m_Pipeline.m_SPHAndIntegrateOnGPU)
    {
        m_Pimpl->m_SmoothedParticleHydrodynamics.Initialize(m_Pimpl->m_VerletIntegration.GetParticlePositions(),
                                               positionsCount);
    }
    Timer::GetInstance()->StopTimerProfile("Intit SPH");
//    m_Pimpl->m_ParticlesAccelerator.Initialize();
    Timer::GetInstance()->StopTimerProfile("Init Physics");

}

void PhysicsParticle::InitializeOpenCL( ID3D11Device *d3D11Device /*= NULL*/,
                                        ID3D11Buffer *d3D11buffer /*= NULL*/)
{
    Timer::GetInstance()->StartTimerProfile();
    assert( ! (m_Pimpl->m_ParticlesGPU.IsUsingInteroperability() && m_Pimpl->m_ParticlesGPU.IsUsingCPU() ));
    
    int status = m_Pimpl->m_ParticlesGPU.Setup(d3D11Device);
    UNUSED_PARAMETER(status);
    assert(status == CL_SUCCESS);

    Timer::GetInstance()->StopTimerProfile("Setup openCL");

    Timer::GetInstance()->StartTimerProfile();

    status = m_Pimpl->m_ParticlesGPU.Initialize(  m_Pimpl->m_VerletIntegration.GetParticlesCount(),
                                        m_Pimpl->m_ParticlesSpring.GetSpringsCount(), 
                                        m_Pimpl->m_ParticlesAccelerator.GetAcceleratorsCount(),
                                        m_Pimpl->m_Pipeline ,d3D11buffer);
    UNUSED_PARAMETER(status);
    assert(status == CL_SUCCESS);


    InitializeOpenClData();
    Timer::GetInstance()->StopTimerProfile("Init openCL");
    
}

// Release memory
void PhysicsParticle::Release()
{
    m_Pimpl->m_ParticlesSpring.Release();
    m_Pimpl->m_ParticlesCollider.Release();
    m_Pimpl->m_ParticlesAccelerator.Release();
}


void PhysicsParticle::InitializeOpenClData()
{
    if (m_Pimpl->m_Pipeline.m_AcceleratorOnGPU)
    {
        assert(m_Pimpl->m_ParticlesAccelerator.GetAcceleratorsCount() > 0);


        m_Pimpl->m_ParticlesGPU.UpdateInputAccelerator(  m_Pimpl->m_VerletIntegration.GetParticlePositions(), 
                                                m_Pimpl->m_VerletIntegration.GetParticlePreviousPositions(),
                                                m_Pimpl->m_VerletIntegration.GetParticlesCount(),
                                                m_Pimpl->m_ParticlesAccelerator.GetAccelerators(), 
                                                m_Pimpl->m_ParticlesAccelerator.GetAcceleratorsCount());
        int status = m_Pimpl->m_ParticlesGPU.InitializeKernelAccelerator();
        UNUSED_PARAMETER(status);
        assert(status == CL_SUCCESS);
    }
    if (m_Pimpl->m_Pipeline.m_CollisionOnGPU)
    {

        m_Pimpl->m_ParticlesGPU.UpdateInputCollision(    m_Pimpl->m_VerletIntegration.GetParticlePositions(), 
                                                m_Pimpl->m_VerletIntegration.GetParticlePreviousPositions(),
                                                m_Pimpl->m_VerletIntegration.GetParticlesCount(),
                                                m_Pimpl->m_ParticlesCollider.GetOutsideSpheres(), m_Pimpl->m_ParticlesCollider.GetOutsideSpheresCount(),
                                                m_Pimpl->m_ParticlesCollider.GetInsideSpheres(), m_Pimpl->m_ParticlesCollider.GetInsideSpheresCount(),
                                                m_Pimpl->m_ParticlesCollider.GetInsideAabbs(), m_Pimpl->m_ParticlesCollider.GetInsideAabbsCount());
                                            

        int status = m_Pimpl->m_ParticlesGPU.InitializeKernelCollision();
        UNUSED_PARAMETER(status);
        assert(status == CL_SUCCESS);
    }
    if (m_Pimpl->m_Pipeline.m_IsCreatingGridOnGPU)
    {
         m_Pimpl->m_ParticlesGPU.UpdateInputCreateGrid(  m_Pimpl->m_VerletIntegration.GetParticlePositions(), 
                                                m_Pimpl->m_ParticlesAccelerator.GetAccelerations(),
                                                (int*)m_Pimpl->m_Grid3D.GetParticleCellOrder(),
                                                (int*)m_Pimpl->m_Grid3D.GetParticleCellOrderBuffer(),
                                                m_Pimpl->m_Grid3D.GetGridInfo());


        int status = m_Pimpl->m_ParticlesGPU.InitializeCreateGrid();
        UNUSED_PARAMETER(status);
        assert(status == CL_SUCCESS);
    
    }
    if (m_Pimpl->m_Pipeline.m_SPHAndIntegrateOnGPU)
    {
        assert(m_Pimpl->m_Pipeline.m_IsCreatingGridOnGPU && "Must use a grid for SPH !");
        // assert (m_Pimpl->m_ParticlesCollider.GetOutsideSpheresCount() > 0 && m_Pimpl->m_ParticlesCollider.GetInsideAabbsCount() > 0);

        m_Pimpl->m_ParticlesGPU.UpdateInputSPH(  m_Pimpl->m_VerletIntegration.GetParticlePositions(), 
                                        m_Pimpl->m_VerletIntegration.GetParticlePreviousPositions(), 
                                        m_Pimpl->m_SmoothedParticleHydrodynamics.GetPreviousDensity(), 
                                        (int*)m_Pimpl->m_Grid3D.GetParticleCellOrder(), 
                                        m_Pimpl->m_Grid3D.GetGridInfo(),
                                        m_Pimpl->m_SmoothedParticleHydrodynamics.GetParameters());
                                            

        int status = m_Pimpl->m_ParticlesGPU.InitializeKernelSPH();
        UNUSED_PARAMETER(status);
        assert(status == CL_SUCCESS);
    }
    if (m_Pimpl->m_Pipeline.m_SpringOnGPU)
    {
        m_Pimpl->m_ParticlesGPU.UpdateInputSpring(   m_Pimpl->m_VerletIntegration.GetParticlePositions(), 
                                        m_Pimpl->m_VerletIntegration.GetParticlesCount(),
                                        m_Pimpl->m_ParticlesSpring.GetSprings(), 
                                        m_Pimpl->m_ParticlesSpring.GetSpringsCount());

        int status = m_Pimpl->m_ParticlesGPU.InitializeKernelSpring();

        UNUSED_PARAMETER(status);
        assert(status == CL_SUCCESS);
    }
    if (m_Pimpl->m_Pipeline.m_IsUsingAnimation)
    {
        assert(m_Pimpl->m_EndsAnimation != NULL);

        m_Pimpl->m_ParticlesGPU.UpdateInputAnimation(   m_Pimpl->m_VerletIntegration.GetParticlePositions(), 
                                                        m_Pimpl->m_EndsAnimation,
                                                        m_Pimpl->m_VerletIntegration.GetParticlesCount());


    int status = m_Pimpl->m_ParticlesGPU.InitializeKernelAnimation();

    UNUSED_PARAMETER(status);
    assert(status == CL_SUCCESS);

    }
}

void PhysicsParticle::CreateGridOnGPU()
{
    Timer::GetInstance()->StartTimerProfile();


    m_Pimpl->m_ParticlesGPU.UpdateInputCreateGrid(   m_Pimpl->m_VerletIntegration.GetParticlePositions(), 
                                            m_Pimpl->m_ParticlesAccelerator.GetAccelerations(),
                                            (int*)m_Pimpl->m_Grid3D.GetParticleCellOrder(),
                                            (int*)m_Pimpl->m_Grid3D.GetParticleCellOrderBuffer(),
                                            m_Pimpl->m_Grid3D.GetGridInfo());


    int status = m_Pimpl->m_ParticlesGPU.runKernelCreateGrid();
    UNUSED_PARAMETER(status);
    assert(status == CL_SUCCESS);
    
    Timer::GetInstance()->StopTimerProfile("Create Grid on GPU");
}

void PhysicsParticle::CollisionOnGPU()
{
    Timer::GetInstance()->StartTimerProfile();

    m_Pimpl->m_ParticlesGPU.UpdateInputCollision(    m_Pimpl->m_VerletIntegration.GetParticlePositions(), 
                                            m_Pimpl->m_VerletIntegration.GetParticlePreviousPositions(),
                                            m_Pimpl->m_VerletIntegration.GetParticlesCount(),
                                            m_Pimpl->m_ParticlesCollider.GetOutsideSpheres(), m_Pimpl->m_ParticlesCollider.GetOutsideSpheresCount(), 
                                            m_Pimpl->m_ParticlesCollider.GetInsideSpheres(), m_Pimpl->m_ParticlesCollider.GetInsideSpheresCount(), 
                                            m_Pimpl->m_ParticlesCollider.GetInsideAabbs(), m_Pimpl->m_ParticlesCollider.GetInsideAabbsCount());
                                            

    int status = m_Pimpl->m_ParticlesGPU.runKernelCollision();
    UNUSED_PARAMETER(status);
    assert(status == CL_SUCCESS);


    Timer::GetInstance()->StopTimerProfile("Collision on GPU");
}

void PhysicsParticle::SimuateSPHIntegrateOnGPU()
{
    Timer::GetInstance()->StartTimerProfile();

    m_Pimpl->m_ParticlesGPU.UpdateInputSPH(  m_Pimpl->m_VerletIntegration.GetParticlePositions(), 
                                    m_Pimpl->m_VerletIntegration.GetParticlePreviousPositions(), 
                                    m_Pimpl->m_SmoothedParticleHydrodynamics.GetPreviousDensity(), 
                                    (int*)m_Pimpl->m_Grid3D.GetParticleCellOrder(), 
                                    m_Pimpl->m_Grid3D.GetGridInfo(),
                                    m_Pimpl->m_SmoothedParticleHydrodynamics.GetParameters());


    int status = m_Pimpl->m_ParticlesGPU.runKernelSPH();
    UNUSED_PARAMETER(status);
    assert(status == CL_SUCCESS);
    
    m_Pimpl->m_VerletIntegration.SwapPositionBuffer();


    Timer::GetInstance()->StopTimerProfile("SPH Integrate on GPU");
}

void PhysicsParticle::SolveSpringOnGPU()
{
    Timer::GetInstance()->StartTimerProfile();

    m_Pimpl->m_ParticlesGPU.UpdateInputSpring(   m_Pimpl->m_VerletIntegration.GetParticlePositions(), 
                                        m_Pimpl->m_VerletIntegration.GetParticlesCount(),
                                        m_Pimpl->m_ParticlesSpring.GetSprings(), 
                                        m_Pimpl->m_ParticlesSpring.GetSpringsCount());

    int status = m_Pimpl->m_ParticlesGPU.runKernelSpring();

    UNUSED_PARAMETER(status);
    assert(status == CL_SUCCESS);

    Timer::GetInstance()->StopTimerProfile("Spring on GPU");
}


void PhysicsParticle::AcceleratorsOnGPU()
{
    Timer::GetInstance()->StartTimerProfile();


    assert (m_Pimpl->m_ParticlesAccelerator.GetAcceleratorsCount() > 0);

    m_Pimpl->m_ParticlesGPU.UpdateInputAccelerator(  m_Pimpl->m_VerletIntegration.GetParticlePositions(), 
                                            m_Pimpl->m_VerletIntegration.GetParticlePreviousPositions(),
                                            m_Pimpl->m_VerletIntegration.GetParticlesCount(),
                                            m_Pimpl->m_ParticlesAccelerator.GetAccelerators(), 
                                            m_Pimpl->m_ParticlesAccelerator.GetAcceleratorsCount());
        

    int status = m_Pimpl->m_ParticlesGPU.runKernelAccelerator();

    UNUSED_PARAMETER(status);
    assert(status == CL_SUCCESS);

    m_Pimpl->m_VerletIntegration.SwapPositionBuffer();

    Timer::GetInstance()->StopTimerProfile("Accelerators on GPU");
}

void PhysicsParticle::Animate()
{
    Timer::GetInstance()->StartTimerProfile();

    assert(m_Pimpl->m_EndsAnimation != NULL);
    m_Pimpl->m_ParticlesGPU.UpdateInputAnimation(   m_Pimpl->m_VerletIntegration.GetParticlePositions(), 
                                                    m_Pimpl->m_EndsAnimation,
                                                    m_Pimpl->m_VerletIntegration.GetParticlesCount());


    int status = m_Pimpl->m_ParticlesGPU.runKernelAnimation();

    UNUSED_PARAMETER(status);
    assert(status == CL_SUCCESS);

    Timer::GetInstance()->StopTimerProfile("Animation");
}

void PhysicsParticle::Simulate()
{

    Timer::GetInstance()->StartTimerProfile();

    // Create grid
    
    if (m_Pimpl->m_Pipeline.m_IsCreatingGridOnGPU)
    {
        CreateGridOnGPU();
    }
    else if (m_IsUsingGrid3D)
    {
        m_Pimpl->m_Grid3D.Initialize(m_Pimpl->m_VerletIntegration.GetParticlePositions(), 
                            m_Pimpl->m_VerletIntegration.GetParticlesCount());
    }



    if (m_Pimpl->m_Pipeline.m_AcceleratorOnGPU)
    {
        AcceleratorsOnGPU();
    }
    else if (m_IsUsingAccelerator)
    {
        m_Pimpl->m_ParticlesAccelerator.Accelerate(  m_Pimpl->m_VerletIntegration.GetParticlePositions(), 
                                            m_Pimpl->m_VerletIntegration.GetParticlesCount());

        m_Pimpl->m_VerletIntegration.AccumateAccelerations(  m_Pimpl->m_ParticlesAccelerator.GetAccelerations(),
                                                    m_Pimpl->m_VerletIntegration.GetParticlesCount());
    }

    

    
    if (m_Pimpl->m_Pipeline.m_SPHAndIntegrateOnGPU)
    {
        assert(m_Pimpl->m_Pipeline.m_IsCreatingGridOnGPU && "Must use a grid for SPH !");
        SimuateSPHIntegrateOnGPU();
    }
    else if (m_SPHSimulation)
    {
        assert(m_IsUsingGrid3D && "Must use a grid for SPH !");
        
        m_Pimpl->m_SmoothedParticleHydrodynamics.Simulate(   m_Pimpl->m_VerletIntegration.GetParticlePositions(), 
                                                    m_Pimpl->m_VerletIntegration.GetParticlesCount());

        m_Pimpl->m_VerletIntegration.AccumateAccelerations(  m_Pimpl->m_SmoothedParticleHydrodynamics.GetParticleAccelerations(),
                                                    m_Pimpl->m_SmoothedParticleHydrodynamics.GetParticlesCount());
    }


    if (!m_Pimpl->m_Pipeline.m_SPHAndIntegrateOnGPU && !m_Pimpl->m_Pipeline.m_AcceleratorOnGPU)
    {
        if (m_ContinuousIntegration)
        {
            m_Pimpl->m_VerletIntegration.ContinuousIntegration();
        }
        else if (m_IsIntegrating)
        {
            m_Pimpl->m_VerletIntegration.Integration();
        }
    }

    if (m_Pimpl->m_Pipeline.m_SpringOnGPU)
    {
        SolveSpringOnGPU();
    }
    else if (m_IsSolvingSpring)
    {
        m_Pimpl->m_ParticlesSpring.Solve(    m_Pimpl->m_VerletIntegration.GetParticlePositions(), 
                                    m_Pimpl->m_VerletIntegration.GetParticlesCount());
    }

    if (m_Pimpl->m_Pipeline.m_IsUsingAnimation)
    {
        Animate();
    }

    if (m_Pimpl->m_Pipeline.m_CollisionOnGPU)
    {
        CollisionOnGPU();
    }
    else if (m_IsColliding)
    {
        m_Pimpl->m_ParticlesCollider.SatisfyCollisions(  m_Pimpl->m_VerletIntegration.GetParticlePositions(), 
                                                m_Pimpl->m_VerletIntegration.GetParticlesCount());
    }
    
    Timer::GetInstance()->StopTimerProfile("Physics simulation");
}


// Collision
void PhysicsParticle::AddInsideAabb(const vrAabb& aabb)
{
    m_Pimpl->m_ParticlesCollider.AddInsideAabb(*reinterpret_cast<const Aabb*>(&aabb));
}

void PhysicsParticle::AddOutsideAabb(const vrAabb& aabb)
{
    m_Pimpl->m_ParticlesCollider.AddOutsideAabb(*reinterpret_cast<const Aabb*>(&aabb));
}

vrAabb* PhysicsParticle::GetInsideAabbs()
{
    return reinterpret_cast<vrAabb*>(m_Pimpl->m_ParticlesCollider.GetInsideAabbs());
}

int PhysicsParticle::GetInsideAabbsCount() const
{
    return m_Pimpl->m_ParticlesCollider.GetInsideAabbsCount();
}

void PhysicsParticle::AddOutsideSphere(const vrSphere& sphere)
{
    m_Pimpl->m_ParticlesCollider.AddOutsideSphere(*reinterpret_cast<const Sphere*>(&sphere));
}

vrSphere* PhysicsParticle::GetOutsideSpheres()
{
    return reinterpret_cast<vrSphere*>(m_Pimpl->m_ParticlesCollider.GetOutsideSpheres());
}

int PhysicsParticle::GetOutsideSpheresCount() const
{
    return m_Pimpl->m_ParticlesCollider.GetOutsideSpheresCount();
}

void PhysicsParticle::AddInsideSphere(const vrSphere& sphere)
{
    m_Pimpl->m_ParticlesCollider.AddInsideSphere(*reinterpret_cast<const Sphere*>(&sphere));
}

vrSphere* PhysicsParticle::GetInsideSpheres()
{
    return reinterpret_cast<vrSphere*>(m_Pimpl->m_ParticlesCollider.GetInsideSpheres());
}

int PhysicsParticle::GetInsideSpheresCount() const
{
    return m_Pimpl->m_ParticlesCollider.GetInsideSpheresCount();
}

// Spring
void PhysicsParticle::AddSpring(const vrSpring& spring)
{
    m_Pimpl->m_ParticlesSpring.AddSpring(*reinterpret_cast<const Spring*>(&spring));
}

// Accelerators
void PhysicsParticle::AddAccelerator(const vrAccelerator & accelerator)
{
    m_Pimpl->m_ParticlesAccelerator.AddAccelerator(*reinterpret_cast<const Accelerator*>(&accelerator));
}

void PhysicsParticle::ClearAccelerators()
{
    m_Pimpl->m_ParticlesAccelerator.ClearAccelerators();
}

void PhysicsParticle::UpdateAccelerator(int index, const vrAccelerator& accelerator)
{
    assert(accelerator.m_Type < vrAccelerator::ACCELRATOR_TYPES_COUNT);
    m_Pimpl->m_ParticlesAccelerator.UpdateAccelerator(index, *reinterpret_cast<const Accelerator*>(&accelerator));
}

void PhysicsParticle::SetEnableGridOnGPU(bool isCreatingGridOnGPU)
{
    m_Pimpl->m_Pipeline.m_IsCreatingGridOnGPU = isCreatingGridOnGPU;
}

void PhysicsParticle::SetEnableSPHAndIntegrateOnGPU(bool sphAndIntegrateOnGPU)
{
    m_Pimpl->m_Pipeline.m_SPHAndIntegrateOnGPU = sphAndIntegrateOnGPU;
}

void PhysicsParticle::SetEnableCollisionOnGPU(bool collisionOnGPU)
{
    m_Pimpl->m_Pipeline.m_CollisionOnGPU = collisionOnGPU;
}

void PhysicsParticle::SetEnableSpringOnGPU(bool springOnGPU)
{
    m_Pimpl->m_Pipeline.m_SpringOnGPU = springOnGPU;
}

void PhysicsParticle::SetEnableAcceleratorOnGPU(bool acceleratorOnGPU)
{
    m_Pimpl->m_Pipeline.m_AcceleratorOnGPU = acceleratorOnGPU;
}

void PhysicsParticle::SetEnableAnimation(bool enableAnimation)
{
    m_Pimpl->m_Pipeline.m_IsUsingAnimation = enableAnimation;
}

void PhysicsParticle::SetClothCount(int clothCount)
{
    m_Pimpl->m_ParticlesGPU.SetClothCount(clothCount);
}
void PhysicsParticle::SetAnimationTime(float animationTime)
{
    m_Pimpl->m_ParticlesGPU.SetAnimationTime(animationTime);
}

void PhysicsParticle::SetIsUsingInteroperability(bool isUsingInteroperability)
{
    m_Pimpl->m_ParticlesGPU.SetIsUsingInteroperability(isUsingInteroperability);
}

bool PhysicsParticle::IsUsingInteroperability() const
{
    return m_Pimpl->m_ParticlesGPU.IsUsingInteroperability();
}

void PhysicsParticle::SetIsUsingCPU(bool isUsingCPU)
{
    m_Pimpl->m_ParticlesGPU.SetIsUsingCPU(isUsingCPU);
}
