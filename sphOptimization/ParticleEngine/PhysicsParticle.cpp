
#include "PhysicsParticle.h"

#include <slmath/slmath.h>

#include "ParticlesCollider.h"
#include "SmoothedParticleHydrodynamics.h"
#include "VerletIntegration.h"
#include "Grid3D.h"
#include "ParticlesSpring.h"
#include "ParticlesAccelerator.h"
#include "ParticlesGPU/ParticlesGPU.hpp"

#include "Utility/Timer.h"

namespace
{
    Grid3D m_Grid3D;
    SmoothedParticleHydrodynamics   m_SmoothedParticleHydrodynamics(&m_Grid3D);
    ParticlesCollider               m_ParticlesCollider;
    VerletIntegration               m_VerletIntegration;
    ParticlesSpring                 m_ParticlesSpring;
    ParticlesAccelerator            m_ParticlesAccelerator;
    ParticlesGPU                    m_ParticlesGPU("GPU");
}

PhysicsParticle::PhysicsParticle() :    // Accessible parameters
                                        m_IsCreatingGridOnGPU(false),
                                        m_SPHAndIntegrateOnGPU(false),
                                        m_CollisionOnGPU(false),
                                        m_SpringOnGPU(false),
                                        m_AcceleratorOnGPU(false),
                                        // Private parameters disabled
                                        m_IsUsingGrid3D         (false),
                                        m_SPHSimulation         (false),
                                        m_ContinuousIntegration (false),
                                        m_IsUsingAccelerator    (false),
                                        m_IsIntegrating         (false),
                                        m_IsSolvingSpring       (false),
                                        m_IsColliding           (false)
{
}

PhysicsParticle::~PhysicsParticle()
{
    if (m_ParticlesGPU.cleanup() != SDK_SUCCESS)
    {
        assert(0);
    }
}

vrVec4 *PhysicsParticle::GetParticlePositions() const
{
    return reinterpret_cast<vrVec4*>(m_VerletIntegration.GetParticlePositions());
}

vrVec4 *PhysicsParticle::GetPreviousPositions() const
{
    return reinterpret_cast<vrVec4*>(m_VerletIntegration.GetParticlePreviousPositions());
}

int PhysicsParticle::GetParticlesCount() const
{
    return m_VerletIntegration.GetParticlesCount();
}

// Properies Setters
void PhysicsParticle::SetParticlesMass(float mass)
{
    m_SmoothedParticleHydrodynamics.SetParticlesMass(mass);
}

void PhysicsParticle::SetParticlesGazConstant(float gazConstant)
{
    m_SmoothedParticleHydrodynamics.SetParticlesGazConstant(gazConstant);
}

void PhysicsParticle::SetParticlesViscosity(float viscosity)
{
    m_SmoothedParticleHydrodynamics.SetParticlesMuViscosityt(viscosity);
}

void PhysicsParticle::SetParticlesAcceleration(const vrVec4 &acceleration)
{
    m_VerletIntegration.SetCommonAcceleration(*reinterpret_cast<const slmath::vec4*>(&acceleration));
}

void PhysicsParticle::SetDamping(float damping)
{
    m_VerletIntegration.SetDamping(damping);
}

// Properies Getters
float PhysicsParticle::GetParticlesMass() const
{
    return m_SmoothedParticleHydrodynamics.GetParameters().m_Mass;
}

float PhysicsParticle::GetParticlesGazConstant() const
{
    return m_SmoothedParticleHydrodynamics.GetParameters().m_GazConstant;
}

float PhysicsParticle::GetParticlesViscosity() const
{
    return m_SmoothedParticleHydrodynamics.GetParameters().m_MuViscosity;
}


void PhysicsParticle::CreateGrid()
{
     m_Grid3D.Initialize(   m_VerletIntegration.GetParticlePositions(), 
                            m_VerletIntegration.GetParticlesCount());
}

void PhysicsParticle::CreateGrid(vrVec4 *positions, int positionsCount)
{
    m_Grid3D.Initialize(reinterpret_cast<slmath::vec4*>(positions), positionsCount);
}

int PhysicsParticle::GetNeighbors(int currentIndex,  int *neighbors, int neighborsMaxCount)
{
    return m_Grid3D.GetNeighbors(currentIndex, neighbors, neighborsMaxCount);
}

void PhysicsParticle::Initialize(vrVec4 *positions,
                                 int positionsCount)
{
    Timer::GetInstance()->StartTimerProfile();
    
    // Initialze grid to allocate memory
    CreateGrid(positions, positionsCount);

    Timer::GetInstance()->StartTimerProfile();
    m_VerletIntegration.Initialize(reinterpret_cast<slmath::vec4*>(positions), positionsCount);
    m_VerletIntegration.SetGrid3D(&m_Grid3D);
    m_ParticlesAccelerator.AllocateAccelerations(positionsCount);
    Timer::GetInstance()->StopTimerProfile("Init Verlet");
    Timer::GetInstance()->StartTimerProfile();
    m_SmoothedParticleHydrodynamics.Initialize(m_VerletIntegration.GetParticlePositions(),
                                               positionsCount);
    Timer::GetInstance()->StopTimerProfile("Intit SPH");
    m_ParticlesAccelerator.Initialize();
    Timer::GetInstance()->StopTimerProfile("Init Physics");

}

void PhysicsParticle::InitializeOpenCL( ID3D11Device *d3D11Device /*= NULL*/,
                                        ID3D11Buffer *d3D11buffer /*= NULL*/)
{

    Timer::GetInstance()->StartTimerProfile();
    assert( ! (m_ParticlesGPU.IsUsingInteroperability() && m_ParticlesGPU.IsUsingCPU() ));


    int status = m_ParticlesGPU.setup(  m_VerletIntegration.GetParticlesCount(),
                                        m_ParticlesSpring.GetSpringsCount(), 
                                        m_ParticlesAccelerator.GetAcceleratorsCount(),
                                        d3D11Device, d3D11buffer);
    UNUSED_PARAMETER(status);
    assert(status == SDK_SUCCESS);


    InitializeOpenClData();

    Timer::GetInstance()->StopTimerProfile("Setup openCL");
}

// Release memory
void PhysicsParticle::Release()
{
    m_ParticlesSpring.Release();
    m_ParticlesCollider.Release();
    m_ParticlesAccelerator.Release();
}


void PhysicsParticle::InitializeOpenClData()
{
    if (m_AcceleratorOnGPU)
    {
        assert(m_ParticlesAccelerator.GetAcceleratorsCount() > 0);


        m_ParticlesGPU.UpdateInputAccelerator(  m_VerletIntegration.GetParticlePositions(), 
                                                m_VerletIntegration.GetParticlePreviousPositions(),
                                                m_VerletIntegration.GetParticlesCount(),
                                                m_ParticlesAccelerator.GetAccelerators(), 
                                                m_ParticlesAccelerator.GetAcceleratorsCount());
        int status = m_ParticlesGPU.InitializeKernelAccelerator();
        UNUSED_PARAMETER(status);
        assert(status == SDK_SUCCESS);
    }
    if (m_CollisionOnGPU)
    {
        assert(m_ParticlesCollider.GetOutsideSpheresCount() > 0 && m_ParticlesCollider.GetInsideAabbsCount() > 0);
        

        m_ParticlesGPU.UpdateInputCollision(    m_VerletIntegration.GetParticlePositions(), 
                                                m_VerletIntegration.GetParticlePreviousPositions(),
                                                m_VerletIntegration.GetParticlesCount(),
                                                m_ParticlesCollider.GetOutsideSpheres(), m_ParticlesCollider.GetOutsideSpheresCount(), 
                                                m_ParticlesCollider.GetInsideAabbs(), m_ParticlesCollider.GetInsideAabbsCount());
                                            

        int status = m_ParticlesGPU.InitializeKernelCollision();
        UNUSED_PARAMETER(status);
        assert(status == SDK_SUCCESS);
    }
    if (m_IsCreatingGridOnGPU)
    {
         m_ParticlesGPU.UpdateInputCreateGrid(  m_VerletIntegration.GetParticlePositions(), 
                                                m_ParticlesAccelerator.GetAccelerations(),
                                                (int*)m_Grid3D.GetParticleCellOrder(),
                                                (int*)m_Grid3D.GetParticleCellOrderBuffer(),
                                                m_Grid3D.GetGridInfo());


        int status = m_ParticlesGPU.InitializeCreateGrid();
        UNUSED_PARAMETER(status);
        assert(status == SDK_SUCCESS);
    
    }
    if (m_SPHAndIntegrateOnGPU)
    {
        assert(m_IsCreatingGridOnGPU && "Must use a grid for SPH !");
        assert (m_ParticlesCollider.GetOutsideSpheresCount() > 0 && m_ParticlesCollider.GetInsideAabbsCount() > 0);

        m_ParticlesGPU.UpdateInputSPH(  m_VerletIntegration.GetParticlePositions(), 
                                        m_VerletIntegration.GetParticlePreviousPositions(), 
                                        m_SmoothedParticleHydrodynamics.GetPreviousDensity(), 
                                        (int*)m_Grid3D.GetParticleCellOrder(), 
                                        m_Grid3D.GetGridInfo(),
                                        m_SmoothedParticleHydrodynamics.GetParameters());
                                            

        int status = m_ParticlesGPU.InitializeKernelSPH();
        UNUSED_PARAMETER(status);
        assert(status == SDK_SUCCESS);
    }
    if (m_SpringOnGPU)
    {
        m_ParticlesGPU.UpdateInputSpring(   m_VerletIntegration.GetParticlePositions(), 
                                        m_VerletIntegration.GetParticlesCount(),
                                        m_ParticlesSpring.GetSprings(), 
                                        m_ParticlesSpring.GetSpringsCount());

        int status = m_ParticlesGPU.InitializeKernelSpring();

        UNUSED_PARAMETER(status);
        assert(status == SDK_SUCCESS);
    }
}

void PhysicsParticle::CreateGridOnGPU()
{
    Timer::GetInstance()->StartTimerProfile();


    m_ParticlesGPU.UpdateInputCreateGrid(   m_VerletIntegration.GetParticlePositions(), 
                                            m_ParticlesAccelerator.GetAccelerations(),
                                            (int*)m_Grid3D.GetParticleCellOrder(),
                                            (int*)m_Grid3D.GetParticleCellOrderBuffer(),
                                            m_Grid3D.GetGridInfo());


    int status = m_ParticlesGPU.runKernelCreateGrid();
    UNUSED_PARAMETER(status);
    assert(status == SDK_SUCCESS);
    
    Timer::GetInstance()->StopTimerProfile("Create Grid on GPU");
}

void PhysicsParticle::CollisionOnGPU()
{
    Timer::GetInstance()->StartTimerProfile();

    assert (m_ParticlesCollider.GetOutsideSpheresCount() > 0 && m_ParticlesCollider.GetInsideAabbsCount() > 0);

    m_ParticlesGPU.UpdateInputCollision(    m_VerletIntegration.GetParticlePositions(), 
                                            m_VerletIntegration.GetParticlePreviousPositions(),
                                            m_VerletIntegration.GetParticlesCount(),
                                            m_ParticlesCollider.GetOutsideSpheres(), m_ParticlesCollider.GetOutsideSpheresCount(), 
                                            m_ParticlesCollider.GetInsideAabbs(), m_ParticlesCollider.GetInsideAabbsCount());
                                            

    int status = m_ParticlesGPU.runKernelCollision();
    UNUSED_PARAMETER(status);
    assert(status == SDK_SUCCESS);


    Timer::GetInstance()->StopTimerProfile("Collision on GPU");
}

void PhysicsParticle::SimuateSPHIntegrateOnGPU()
{
    Timer::GetInstance()->StartTimerProfile();

    m_ParticlesGPU.UpdateInputSPH(  m_VerletIntegration.GetParticlePositions(), 
                                    m_VerletIntegration.GetParticlePreviousPositions(), 
                                    m_SmoothedParticleHydrodynamics.GetPreviousDensity(), 
                                    (int*)m_Grid3D.GetParticleCellOrder(), 
                                    m_Grid3D.GetGridInfo(),
                                    m_SmoothedParticleHydrodynamics.GetParameters());


    int status = m_ParticlesGPU.runKernelSPH();
    UNUSED_PARAMETER(status);
    assert(status == SDK_SUCCESS);
    
    m_VerletIntegration.SwapPositionBuffer();


    Timer::GetInstance()->StopTimerProfile("SPH Integrate on GPU");

}

void PhysicsParticle::SolveSpringOnGPU()
{
    Timer::GetInstance()->StartTimerProfile();

    m_ParticlesGPU.UpdateInputSpring(   m_VerletIntegration.GetParticlePositions(), 
                                        m_VerletIntegration.GetParticlesCount(),
                                        m_ParticlesSpring.GetSprings(), 
                                        m_ParticlesSpring.GetSpringsCount());

    int status = m_ParticlesGPU.runKernelSpring();

    UNUSED_PARAMETER(status);
    assert(status == SDK_SUCCESS);

    Timer::GetInstance()->StopTimerProfile("Spring on GPU");
}


void PhysicsParticle::AcceleratorsOnGPU()
{
    Timer::GetInstance()->StartTimerProfile();


    assert (m_ParticlesAccelerator.GetAcceleratorsCount() > 0);

    m_ParticlesGPU.UpdateInputAccelerator(  m_VerletIntegration.GetParticlePositions(), 
                                            m_VerletIntegration.GetParticlePreviousPositions(),
                                            m_VerletIntegration.GetParticlesCount(),
                                            m_ParticlesAccelerator.GetAccelerators(), 
                                            m_ParticlesAccelerator.GetAcceleratorsCount());
        

    int status = m_ParticlesGPU.runKernelAccelerator();

    UNUSED_PARAMETER(status);
    assert(status == SDK_SUCCESS);

    m_VerletIntegration.SwapPositionBuffer();

    Timer::GetInstance()->StopTimerProfile("Accelerators on GPU");
}

void PhysicsParticle::Simulate()
{

    Timer::GetInstance()->StartTimerProfile();

    // Create grid
    
    if (m_IsCreatingGridOnGPU)
    {
        CreateGridOnGPU();
    }
    else if (m_IsUsingGrid3D)
    {
        m_Grid3D.InitializeFixedGrid(m_VerletIntegration.GetParticlePositions(), 
                            m_VerletIntegration.GetParticlesCount());
    }



    if (m_AcceleratorOnGPU)
    {
        AcceleratorsOnGPU();
    }
    else if (m_IsUsingAccelerator)
    {
        m_ParticlesAccelerator.Accelerate(  m_VerletIntegration.GetParticlePositions(), 
                                            m_VerletIntegration.GetParticlesCount());

        m_VerletIntegration.AccumateAccelerations(  m_ParticlesAccelerator.GetAccelerations(),
                                                    m_VerletIntegration.GetParticlesCount());
    }

    

    
    if (m_SPHAndIntegrateOnGPU)
    {
        assert(m_IsCreatingGridOnGPU && "Must use a grid for SPH !");
        SimuateSPHIntegrateOnGPU();
    }
    else if (m_SPHSimulation)
    {
        assert(m_IsUsingGrid3D && "Must use a grid for SPH !");
        
        m_SmoothedParticleHydrodynamics.Simulate(   m_VerletIntegration.GetParticlePositions(), 
                                                    m_VerletIntegration.GetParticlesCount());

        m_VerletIntegration.AccumateAccelerations(  m_SmoothedParticleHydrodynamics.GetParticleAccelerations(),
                                                    m_SmoothedParticleHydrodynamics.GetParticlesCount());
    }


    if (!m_SPHAndIntegrateOnGPU && !m_AcceleratorOnGPU)
    {
        if (m_ContinuousIntegration)
        {
            m_VerletIntegration.ContinuousIntegration();
        }
        else if (m_IsIntegrating)
        {
            m_VerletIntegration.Integration();
        }
    }

    if (m_SpringOnGPU)
    {
        SolveSpringOnGPU();
    }
    else if (m_IsSolvingSpring)
    {
        m_ParticlesSpring.Solve(    m_VerletIntegration.GetParticlePositions(), 
                                    m_VerletIntegration.GetParticlesCount());
    }

    if (m_CollisionOnGPU)
    {
        CollisionOnGPU();
    }
    else if (m_IsColliding)
    {
        m_ParticlesCollider.SatisfyCollisions(  m_VerletIntegration.GetParticlePositions(), 
                                                m_VerletIntegration.GetParticlesCount());
    }


    
    Timer::GetInstance()->StopTimerProfile("Physics simulation");
}


// Collision
void PhysicsParticle::AddInsideAabb(const vrAabb& aabb)
{
    m_ParticlesCollider.AddInsideAabb(*reinterpret_cast<const Aabb*>(&aabb));
}

void PhysicsParticle::AddOutsideAabb(const vrAabb& aabb)
{
    m_ParticlesCollider.AddOutsideAabb(*reinterpret_cast<const Aabb*>(&aabb));
}

void PhysicsParticle::AddInsideSphere(const vrSphere& sphere)
{
    m_ParticlesCollider.AddInsideSphere(*reinterpret_cast<const Sphere*>(&sphere));
}

void PhysicsParticle::AddOutsideSphere(const vrSphere& sphere)
{
    m_ParticlesCollider.AddOutsideSphere(*reinterpret_cast<const Sphere*>(&sphere));
}

vrAabb* PhysicsParticle::GetInsideAabbs()
{
    return reinterpret_cast<vrAabb*>(m_ParticlesCollider.GetInsideAabbs());
}

vrSphere* PhysicsParticle::GetOutsideSpheres()
{
    return reinterpret_cast<vrSphere*>(m_ParticlesCollider.GetOutsideSpheres());
}

int PhysicsParticle::GetInsideAabbsCount() const
{
    return m_ParticlesCollider.GetInsideAabbsCount();
}

int PhysicsParticle::GetOutsideSpheresCount() const
{
    return m_ParticlesCollider.GetOutsideSpheresCount();
}

// Spring
void PhysicsParticle::AddSpring(const vrSpring& spring)
{
    m_ParticlesSpring.AddSpring(*reinterpret_cast<const Spring*>(&spring));
}

// Accelerators
void PhysicsParticle::AddAccelerator(const vrAccelerator & accelerator)
{
    m_ParticlesAccelerator.AddAccelerator(*reinterpret_cast<const Accelerator*>(&accelerator));
}

void PhysicsParticle::ClearAccelerators()
{
    m_ParticlesAccelerator.ClearAccelerators();
}

void PhysicsParticle::UpdateAccelerator(int index, const vrAccelerator& accelerator)
{
    assert(accelerator.m_Type < vrAccelerator::ACCELRATOR_TYPES_COUNT);
    m_ParticlesAccelerator.UpdateAccelerator(index, *reinterpret_cast<const Accelerator*>(&accelerator));
}

void PhysicsParticle::SetEnableGridOnGPU(bool isCreatingGridOnGPU)
{
    m_IsCreatingGridOnGPU = isCreatingGridOnGPU;
}

void PhysicsParticle::SetEnableSPHAndIntegrateOnGPU(bool sphAndIntegrateOnGPU)
{
    m_SPHAndIntegrateOnGPU = sphAndIntegrateOnGPU;
}

void PhysicsParticle::SetEnableCollisionOnGPU(bool collisionOnGPU)
{
    m_CollisionOnGPU = collisionOnGPU;
}

void PhysicsParticle::SetEnableSpringOnGPU(bool springOnGPU)
{
    m_SpringOnGPU = springOnGPU;
}

void PhysicsParticle::SetEnableAcceleratorOnGPU(bool acceleratorOnGPU)
{
    m_AcceleratorOnGPU = acceleratorOnGPU;
}

void PhysicsParticle::SetClothCount(int clothCount)
{
    m_ParticlesGPU.SetClothCount(clothCount);
}

void PhysicsParticle::SetIsUsingInteroperability(bool isUsingInteroperability)
{
    m_ParticlesGPU.SetIsUsingInteroperability(isUsingInteroperability);
}

bool PhysicsParticle::IsUsingInteroperability() const
{
    return m_ParticlesGPU.IsUsingInteroperability();
}

void PhysicsParticle::SetIsUsingCPU(bool isUsingCPU)
{
    m_ParticlesGPU.SetIsUsingCPU(isUsingCPU);
}
