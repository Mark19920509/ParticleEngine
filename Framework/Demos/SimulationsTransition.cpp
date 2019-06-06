#include "SimulationsTransition.h"
#include "Utility/Timer.h"
#include "../Camera.h"

#include <math.h>
#include <vector>
#include <stdlib.h>


namespace {

typedef SimulationsTransition Demo;

Demo::Event s_Events[] =   {
    // Demo beginning: colorful particles start in front
    // of the screen
    {Demo::eStart,                        10.0f, 0.0f }, //  0:10
    // When the particles go forward to the cloth
    // Here I noticed you have considerd the animation when 
    // particles slow down and change, what you ve done it is really good
    // andkeep it like this. 
    // There is no specific event because I slow down the particles when the 
    // particles reach the clothes
    {Demo::eZoomIn,                       30.0f, 0.0f }, // 0:40
    // The camera rotates around the particles
    {Demo::eRotateAroundCloth,            12.0f, 0.0f }, // 0:52
    // The particles go away from the center of the cloth
    {Demo::eMoveAway,                     30.0f, 0.0f }, // 1:22 // 82
    // Particles start to move a bit faster around the cloth
    {Demo::eRevolution,                   30.0f, 0.0f }, // 1:52 // 112
    // The colorful particles go to the center of the scene 
    // where the galaxy will be created
    {Demo::eGoToForceField,               15.0f, 0.0f }, // 2:07 //127
    // This is doing nothing actually... 
    {Demo::eMoveSpheres,                   1.0f, 0.0f }, // 2:08 //128
    // The strong wind is starting the cloth are moving really fast
    // A music acceleration or something faster could be done
    {Demo::eStrongWind,                   20.0f, 0.0f }, // 2:28 // 148
    // The clothes are detached and start to fly
    {Demo::eClothFlying,                   8.0f, 0.0f }, // 2:36 // 156
    // The particles go down to the force field position
    {Demo::eParticleFollowCamera,          8.0f, 0.0f }, // 2:44 // 164
    // This is a BIG change in the music like it is now,
    // the ground starts to be a big particle system and becomes
    // a galaxy.
    {Demo::eForceField,                   50.0f, 0.0f }, //  3:34// 214
    // Now the sky starts to move away from the center
    {Demo::e2ForceField,                  50.0f, 0.0f }, // 4:24 // 264
    // Here the camera is still turning around the particles, 
    // but they have stop to move here the music should be suddlenly
    // calmer and slower like a soft music
    {Demo::eStopMotion,                   20.0f, 0.0f }, // 4:44 // 284
    // Then the particles are falling, only a simple gravity is applied
    // The music or the ambient sound 
    {Demo::eCollison,                     20.0f, 0.0f }, // 5:04 // 304
    // The particles are attracted by the center 
    {Demo::eAttraction,                    8.0f, 0.0f }, // 5:12 // 312
    // The ground and the sky start to move to the original position
    // like the world take its original shape, this is quite nice and 
    // short effect that could have an additionanl sound
    {Demo::eAnimationForceField2,          8.0f, 0.0f }, // 5:20 // 320
    // Particle water are appearing and animated 
    {Demo::eAnimationWater1,              18.0f, 0.0f }, // 5:38 // 338
    // and now they are simulated like this video : 
    // https://www.youtube.com/watch?v=44K-YTCQWDo
    {Demo::eWater1,                       30.0f, 0.0f }, // 6:08 //368
    // Now these become the start of the end and we see the name 
    // Chris Davis
    {Demo::eAnimationWater2,              18.0f, 0.0f }, //  6:26 // 386
    // The particle explode 
    {Demo::eWater2,                        6.0f, 0.0f }, //  6:32 // 392
    // and they become Elliot Hunter
    {Demo::eAnimationWater3,              18.0f, 0.0f }, //  6:50 // 410
    // and explode again
    {Demo::eWater3,                        6.0f, 0.0f }, //  6:56 // 416
    // and become Vincent ROBERT
    {Demo::eAnimationWater4,              18.0f, 0.0f }, //  7:14 // 434
    // and explode again : The End, end of the music
    // the music become quiter
    {Demo::eWater4,                       15.0f, 0.0f } // 7:24 // 444
    };

}



SimulationsTransition::SimulationsTransition()
 : m_Camera(NULL)
{
}

void SimulationsTransition::Initialize()
{
    m_Events        = &s_Events[0];
    m_CurrentEvent  = &m_Events[0];

    for (int i = 0; i < eStateCount; i++)
    {
        s_Events[i].m_TimeAccumulation = s_Events[i].m_TimeDuration;
        if (i > 0)
        {
            s_Events[i].m_TimeAccumulation += s_Events[i-1].m_TimeAccumulation;
        }
        // DEBUG_OUT(int (s_Events[i].m_TimeAccumulation) / 60<< ":" << int (s_Events[i].m_TimeAccumulation) % 60 << "\n");
    }

    m_InitialFlyingPosition = slmath::vec3(25.0f, 5.0f, 2000.0f);

    m_RenderSpheresCount = 2;

    m_DetachedClothCount = 0;

    CreateCloth();
    
    CreateForceField();
    CreateSky();
    CreateFlyingParticles();
    CreateWater();
    
    m_FramesCount = 0;

    m_Time = 0.0f;
    m_TimeAcumulator = 0.0f;
    m_DeltaT = 1.0f / 60.0f;

    m_Camera->setPointToFollow(*reinterpret_cast<slmath::vec3*>(&m_AcceleratorForceField4.m_Position));
    m_Camera->setDistanceFromTarget(40.0f);
    m_Camera->setAngleHeight(-1.0f);
    m_Camera->setAngleHorizon(3.14159262f);

    m_ParticleVelocity = slmath::vec3(0.0f, 0.0f, 0.0f);

    m_SpeedAccelerator = 140.0f;

    m_LectureSpeed = 1.0f;


    // Inititialize multithreading
    SYSTEM_INFO sysinfo;
    GetSystemInfo( &sysinfo );

    m_CpuCount = 1; //sysinfo.dwNumberOfProcessors - 1;

    m_ThreadData    = new ThreadData[m_CpuCount];
    m_RunMutex      = new HANDLE[m_CpuCount];




    /*const float dt30        = 1.0f / 30.0f;
    float position30        = 0.0f;
    const float dt60        = 1.0f / 60.0f;
    float position60        = 0.0f;

    float speed30           = 100.0f;
    float acceleration30    = 1000.0f;

    float speed60           = speed30;
    float acceleration60    = acceleration30;

    
    
    for (int i = 0; i < 10; i++)
    {
        speed30 += acceleration30 * dt30 * dt30 * 0.5f;
        position30 += speed30 * dt30;
    }

    for (int i = 0; i < 20; i++)
    {
        speed60 += acceleration60 *  dt60 * dt60 * 0.5f;
        position60 += speed60 * dt60;
    }

    acceleration30 = acceleration60;*/
}

void SimulationsTransition::Release()
{
    
    m_PhysicsParticle.Release();
    m_PhysicsParticle2.Release();
    m_PhysicsParticle3.Release();
    m_PhysicsParticle4.Release();
    m_PhysicsParticle5.Release();
    delete []m_Spheres;
}

void SimulationsTransition::InitializeRenderer(DX11Renderer &renderer)
{
    InitializeGraphicsObjectToRender(renderer);
}

void SimulationsTransition::InitializeGraphicsObjectToRender(DX11Renderer &renderer)
{
    renderer.SetIsUsingInteroperability(m_PhysicsParticle2.IsUsingInteroperability());

    renderer.CreateIndexBuffersForCloth(m_SideCloth, m_ClothCount);
    int maxParticlesCount = slmath::max(m_PhysicsParticle.GetParticlesCount(),
                                         m_PhysicsParticle2.GetParticlesCount());

    renderer.InitializeParticles(reinterpret_cast<slmath::vec4*>(m_PhysicsParticle.GetParticlePositions()), maxParticlesCount, 
                                 m_SpheresCount,
                                 m_PhysicsParticle.GetParticlesCount());

    
    m_RendererIndex3 = renderer.InitializeParticles(reinterpret_cast<slmath::vec4*>(m_PhysicsParticle3.GetParticlePositions()), m_PhysicsParticle3.GetParticlesCount(), m_PhysicsParticle3.IsUsingInteroperability());
    m_RendererIndex4 = renderer.InitializeParticles(reinterpret_cast<slmath::vec4*>(m_PhysicsParticle4.GetParticlePositions()), m_PhysicsParticle4.GetParticlesCount(), false);
    m_RendererIndex5 = renderer.InitializeParticles(reinterpret_cast<slmath::vec4*>(m_PhysicsParticle5.GetParticlePositions()), m_PhysicsParticle5.GetParticlesCount(), m_PhysicsParticle5.IsUsingInteroperability());


    m_PhysicsParticle.InitializeOpenCL();

    m_PhysicsParticle2.InitializeOpenCL(renderer.GetID3D11Device(), renderer.GetID3D11Buffer());
    m_PhysicsParticle2.SetEnableCollisionOnGPU(false);
    m_PhysicsParticle2.SetEnableAnimation(false);

    m_PhysicsParticle3.InitializeOpenCL(renderer.GetID3D11Device(), renderer.GetID3D11Buffer(m_RendererIndex3));
    m_PhysicsParticle3.SetEnableAnimation(false);
    m_PhysicsParticle4.InitializeOpenCL();
    m_PhysicsParticle5.InitializeOpenCL(renderer.GetID3D11Device(), renderer.GetID3D11Buffer(m_RendererIndex5));
    m_PhysicsParticle5.SetEnableAcceleratorOnGPU(false);
    m_PhysicsParticle5.SetEnableAnimation(false);
    m_PhysicsParticle5.SetEnableGridOnGPU(false);
    m_PhysicsParticle5.SetEnableSPHAndIntegrateOnGPU(false);
    m_PhysicsParticle5.SetEnableCollisionOnGPU(false);
    
}


void SimulationsTransition::Draw(DX11Renderer &renderer) const
{

    if (m_CurrentEvent->m_State < eForceField)
    {
        renderer.RenderCloth(GetParticlePositions(), (m_PhysicsParticle.GetParticlesCount() / m_ClothCount) * m_ClothActivatedCount);
    }

    renderer.Render(m_RendererIndex4, reinterpret_cast<slmath::vec4 *>( m_PhysicsParticle4.GetParticlePositions()), 
                                                                            m_PhysicsParticle4.GetParticlesCount());

    if (m_CurrentEvent->m_State > eAnimationForceField2)
    {
        renderer.Render(m_RendererIndex5, reinterpret_cast<slmath::vec4 *>( m_PhysicsParticle5.GetParticlePositions()), 
                                                                            m_PhysicsParticle5.GetParticlesCount());
    }


    renderer.Render(reinterpret_cast<slmath::vec4 *>(m_PhysicsParticle2.GetParticlePositions()), 
                    m_PhysicsParticle2.GetParticlesCount());


    renderer.Render(m_RendererIndex3, reinterpret_cast<slmath::vec4 *>( m_PhysicsParticle3.GetParticlePositions()), 
                                                                        m_PhysicsParticle3.GetParticlesCount());
}

void SimulationsTransition::InitActions()
{

    if (m_CurrentEvent->m_State == eZoomIn)
    {
    }
    else if (m_CurrentEvent->m_State == eRotateAroundCloth)
    {
    }
    else if (m_CurrentEvent->m_State == eMoveAway)
    {
        m_AcceleratorForceField4.m_Radius = 1000.0f;
        m_SpeedAccelerator = 90.0f;
    }
    else if (m_CurrentEvent->m_State == eRevolution)
    {
    }
    else if (m_CurrentEvent->m_State == eGoToForceField )
    {
        StartForceField();
    }
    else if (m_CurrentEvent->m_State == eMoveSpheres)
    {
        float fmodAngle = fmod(m_Camera->getAngleHorizon(), 2 * slmath::PI);
        m_CameraDesiredAngle = m_Camera->getAngleHorizon() - fmodAngle - 2 * slmath::PI;
    }
    else if (m_CurrentEvent->m_State == eStrongWind)
    {
        DetachClothes();
    }
    else if (m_CurrentEvent->m_State == eClothFlying)
    {
    }
    else if (m_CurrentEvent->m_State == eParticleFollowCamera)
    {
    }
    else if (m_CurrentEvent->m_State == eForceField)
    {
    }
    else if (m_CurrentEvent->m_State == e2ForceField)
    {
        KillSpeedForceField();
    }
    else if (m_CurrentEvent->m_State == eStopMotion)
    {
        m_LectureSpeed = 1.0f;
        StartForceFieldCollision();    }
    else if (m_CurrentEvent->m_State == eCollison)
    {
        AttractForceField();
    }
    else if (m_CurrentEvent->m_State == eAttraction)
    {
        InitAnimationForceField2();
        InitAnimationSky3();
    }
    else if (m_CurrentEvent->m_State == eAnimationForceField2)
    {
        InitAnimationWater(0);
    }
    else if (m_CurrentEvent->m_State == eAnimationWater1)
    {
        InitWater();
    }
    else if (m_CurrentEvent->m_State == eWater1)
    {
        InitAnimationWater(1);
    }
    else if (m_CurrentEvent->m_State == eAnimationWater2)
    {
        InitWater();
    }
    else if (m_CurrentEvent->m_State == eWater2)
    {
        InitAnimationWater(2);
    }
    else if (m_CurrentEvent->m_State == eAnimationWater3)
    {
        InitWater();
    }
    else if (m_CurrentEvent->m_State == eWater3)
    {
        InitAnimationWater(3);
    }
    else if (m_CurrentEvent->m_State == eAnimationWater4)
    {
        InitWater();
    }
    else if (m_CurrentEvent->m_State == eWater4)
    {
        exit(0);
    }
}

void SimulationsTransition::CheckActionStates()
{
     // States
    if (m_Time > m_CurrentEvent->m_TimeAccumulation)
    {
        InitActions();
        m_CurrentEvent++;
    }
    // Actions
    if (m_CurrentEvent->m_State == eStart)
    {
        MoveSpheres();
        MoveSphere4();
    }
    if (m_CurrentEvent->m_State == eZoomIn)
    {
        MoveAccelerator4();
        FollowParticles();
        MoveClothAcceleratorByParticles4();
        MoveSpheres();
        MoveSphere4();

        if (m_Camera->getPointToFollow().z < 800 )
        {
            Decelerate();
        }
        if (m_Time > m_CurrentEvent->m_TimeAccumulation - m_CurrentEvent->m_TimeDuration * 0.7f )
        {
            m_Camera->setDistanceFromTarget(80.0f);
            m_Camera->setAngleHeight(0.0f);
            m_Camera->setAngleHorizon(0.0f);
        }
    }
    else if (m_CurrentEvent->m_State == eRotateAroundCloth)
    {
        TurnCamera(3.1415926f / 2.0f * 3.0f);
        RotateCamera();
        MoveAccelerator4();
        FollowParticles();
        MoveClothAcceleratorByParticles4();
        MoveSpheres();
        MoveSphere4();
    }
    else if (m_CurrentEvent->m_State == eMoveAway)
    {
        TurnCamera(3.1415926f / 2.0f * 3.0f);
        m_Camera->setDistanceFromTarget(m_Camera->getDistanceFromTarget() + 5.0f * m_DeltaT);
        MoveAcceleratorDirection4();
        FollowParticles();
        MoveClothAcceleratorByParticles4();
        MoveSpheres();
        MoveSphere4();
    }
    else if (m_CurrentEvent->m_State == eRevolution)
    {
        TurnCamera(0.0f, false);
        RevoluteAccelerator4();
        FollowParticles();
        MoveClothAcceleratorByParticles4();
        MoveSpheres();
        MoveSphere4();
    }
    else if (m_CurrentEvent->m_State == eGoToForceField)
    {
        GoToCenter();
        FollowParticles();
        MoveClothAcceleratorByParticles4();
        MoveSpheres();
        MoveSphere4();
    }
    else if (m_CurrentEvent->m_State == eMoveSpheres)
    {
        MoveIntoCloth();
        TurnCamera(0.0f, false);
        RotateCamera();
    }
    else if (m_CurrentEvent->m_State == eStrongWind)
    {
        TurnCamera(m_CameraDesiredAngle);
        RotateCamera2();
        StrongtWind();
    }
    else if (m_CurrentEvent->m_State == eClothFlying)
    {

        //float time = m_Time - m_CurrentEvent->m_TimeAccumulation + m_CurrentEvent->m_TimeDuration;
        //const float clothTime = slmath::clamp(time / (m_CurrentEvent->m_TimeDuration - 3.0f), 0.0f, 1.0f);

        //if (rand() % 50 == 0)
        //{
        //    int clothToDetach = int(clothTime * float(m_ClothCount));

        //    DEBUG_OUT( clothToDetach << " cloth " << clothTime << "\n");
        //    for (int i = m_DetachedClothCount; i < clothToDetach; i++)
        //    {
        //        DetachCloth(m_Indirections[i]);
        //    }
        //    // Force to send data on the GPU
        //    m_PhysicsParticle.InitializeOpenClData();
        //    m_DetachedClothCount = clothToDetach;
        //}

        TurnCamera(m_CameraDesiredAngle);
        RotateCamera2();
        StrongtWind();
    }
    else if (m_CurrentEvent->m_State == eParticleFollowCamera)
    {
        RotateCamera();
        GoToForceField();
        MoveClothAcceleratorByParticles4();
        MoveSpheres();
        MoveSphere4();
    }
    else if (m_CurrentEvent->m_State == eForceField)
    {
        m_Camera->setDistanceFromTarget(m_Camera->getDistanceFromTarget() + 20.0f * m_DeltaT);
        MoveForceField();
        TurnCamera(0.0f, false);
        // MoveFlyingParticlesByForceField();
    }
    else if (m_CurrentEvent->m_State == e2ForceField)
    {
        MoveForceField();
        TurnCamera(0.0f, false);
        // MoveFlyingParticlesByForceField();
    }
    else if (m_CurrentEvent->m_State == eStopMotion)
    {
        if (m_Camera->getDistanceFromTarget() > 100.0f)
        {
            m_Camera->setDistanceFromTarget(m_Camera->getDistanceFromTarget() - 20.0f * m_DeltaT);
        }
        TurnCamera(0.0f, false);
    }
    else if (m_CurrentEvent->m_State == eCollison)
    {
        if (m_Camera->getDistanceFromTarget() > 100.0f)
        {
            m_Camera->setDistanceFromTarget(m_Camera->getDistanceFromTarget() - 5.0f * m_DeltaT);
        }
        MoveCameraToAttraction();
        // MoveFlyingParticlesByForceField();
    }
    else if (m_CurrentEvent->m_State == eAttraction)
    {
        if (m_Camera->getDistanceFromTarget() > 100.0f)
        {
            m_Camera->setDistanceFromTarget(m_Camera->getDistanceFromTarget() - 5.0f * m_DeltaT);
        }
        MoveCameraToAttraction();
        // MoveFlyingParticlesByForceField();
    }
    else if (m_CurrentEvent->m_State == eAnimationForceField2)
    {
        float time = m_Time - m_CurrentEvent->m_TimeAccumulation + m_CurrentEvent->m_TimeDuration;
        const float animationTime = slmath::clamp(time / 6.0f, 0.0f, 1.0f);
        // DEBUG_OUT( time << " timeAnimation " << animationTime << "\n");
        m_PhysicsParticle2.SetAnimationTime(animationTime);
        m_PhysicsParticle3.SetAnimationTime(animationTime);
        TurnCamera(0.0f, false);
        // MoveFlyingParticlesByForceField();
    }
    else if (m_CurrentEvent->m_State == eAnimationWater1)
    {
        if (m_Camera->getDistanceFromTarget() > 50.0f)
        {
            m_Camera->setDistanceFromTarget(m_Camera->getDistanceFromTarget() - 10.0f * m_DeltaT);
        }
        const float animationTime =slmath::clamp( m_Time / 7.0f, 0.0f, 1.0f);
        m_PhysicsParticle5.SetAnimationTime(animationTime);
        TurnCamera(0.0f, false, 0.5f);
        MoveCameraToWater();
    }
    else if (m_CurrentEvent->m_State == eWater1)
    {
        if (m_Camera->getDistanceFromTarget() > 50.0f)
        {
            m_Camera->setDistanceFromTarget(m_Camera->getDistanceFromTarget() - 10.0f * m_DeltaT);
        }
        TurnCamera(0.0f, false, 0.1f);
        MoveCameraToWater();
    }
    else if (m_CurrentEvent->m_State == eAnimationWater2)
    {
        if (m_Camera->getDistanceFromTarget() < 300.0f)
        {
            m_Camera->setDistanceFromTarget(m_Camera->getDistanceFromTarget() + 10.0f * m_DeltaT);
        }
        float time = m_Time - m_CurrentEvent->m_TimeAccumulation + m_CurrentEvent->m_TimeDuration;
        const float animationTime = slmath::clamp( time / 7.0f, 0.0f, 1.0f);
        //DEBUG_OUT( time << " timeAnimation " << animationTime << "\n");
        m_PhysicsParticle5.SetAnimationTime(animationTime);
        TurnCamera(0.0f, false, 0.4f);
        MoveCameraToWater();
    }
    else if (m_CurrentEvent->m_State == eWater2)
    {
        if (m_Camera->getDistanceFromTarget() < 300.0f)
        {
            m_Camera->setDistanceFromTarget(m_Camera->getDistanceFromTarget() + 10.0f * m_DeltaT);
        }
        TurnCamera(0.0f, false, 0.1f);
        MoveCameraToWater();
    }
    else if (m_CurrentEvent->m_State == eAnimationWater3)
    {
        if (m_Camera->getDistanceFromTarget() < 300.0f)
        {
            m_Camera->setDistanceFromTarget(m_Camera->getDistanceFromTarget() + 10.0f * m_DeltaT);
        }
        float time = m_Time - m_CurrentEvent->m_TimeAccumulation + m_CurrentEvent->m_TimeDuration;
        const float animationTime = slmath::clamp( time / 7.0f, 0.0f, 1.0f);
        //DEBUG_OUT( time << " timeAnimation " << animationTime << "\n");
        m_PhysicsParticle5.SetAnimationTime(animationTime);
        TurnCamera(0.0f, false, 0.4f);
        MoveCameraToWater();
    }
    else if (m_CurrentEvent->m_State == eWater3)
    {
        if (m_Camera->getDistanceFromTarget() < 300.0f)
        {
            m_Camera->setDistanceFromTarget(m_Camera->getDistanceFromTarget() + 10.0f * m_DeltaT);
        }
        TurnCamera(0.0f, false, 0.1f);
    }
    else if (m_CurrentEvent->m_State == eAnimationWater4)
    {
        float time = m_Time - m_CurrentEvent->m_TimeAccumulation + m_CurrentEvent->m_TimeDuration;
        const float animationTime = slmath::clamp( time / 7.0f, 0.0f, 1.0f);
        //DEBUG_OUT( time << " timeAnimation " << animationTime << "\n");
        m_PhysicsParticle5.SetAnimationTime(animationTime);
        TurnCamera(0.0f, false, 0.4f);
    }
    else if (m_CurrentEvent->m_State == eWater4)
    {
        TurnCamera(0.0f, false, 0.1f);
    }
}



unsigned int __stdcall SimulateOnCPU(ThreadData *threadData)
{
    /*for (int i = 0; i < 20; i++)
        DEBUG_OUT("-------" << threadData->mutexIndex << "\n");*/
    threadData->physicsParticle->Simulate();
 
    _endthreadex(0);
    return 0;
}


void SimulationsTransition::Simulate(float /*deltaT*/)
{ 

    m_Time = Timer::GetInstance()->GetTimer(0);
    m_DeltaT = 1.0f / 60.0f;
    
    

    //  while (totalTime > m_TimeAcumulator + m_DeltaT)
    {
        //m_DeltaT *= m_LectureSpeed; // m_LectureSpeed * deltaT;
        
        m_FramesCount++;
        
        // DEBUG_OUT("Time :\t" << m_Time << " \t state: " << m_CurrentEvent->m_State << "\n");
        // Check time action and manage event
        CheckActionStates();

        // Lets have the CPU working
        //for (int i = 0; i < 1/*mCpuCount*/; i++)
        //{
        //    unsigned int indexThread;
        //    mThreadData[i].mutexIndex = i;
        //    mThreadData[i].physicsParticle = &m_PhysicsParticle;
        //    mRunMutex[i] = (HANDLE) _beginthreadex( NULL, 0, (unsigned int (__stdcall *)(void *))(&SimulateOnCPU), &mThreadData[i], 0, &indexThread);
        //}

        /*for (int i = 0; i < 10; i++)
            DEBUG_OUT("MAIN MAIN MAIN\n");*/

    //     
    


      //  DEBUG_OUT("END\n");

        if (m_CurrentEvent->m_State >= eForceField && m_CurrentEvent->m_State != eStopMotion && m_CurrentEvent->m_State < eAnimationWater1)
        {
            m_PhysicsParticle2.Simulate();
        }
        if ( (m_CurrentEvent->m_State >= e2ForceField && m_CurrentEvent->m_State < eStopMotion) || m_CurrentEvent->m_State == eAnimationForceField2)
        {
            m_PhysicsParticle3.Simulate();
        }
        if (m_CurrentEvent->m_State < eForceField)
        {
            m_PhysicsParticle.Simulate();
        }
        if (m_CurrentEvent->m_State >= eAnimationForceField2)
        {
            m_PhysicsParticle5.Simulate();
        }

        m_PhysicsParticle4.Simulate();


        //WaitForMultipleObjects(1, sRunMutex, TRUE, INFINITE);
        /*WaitForSingleObject( mRunMutex[0], INFINITE );
        CloseHandle(mRunMutex[0]);*/

        m_TimeAcumulator += m_DeltaT;
    }
}


////////////////////////////////////////////////////////////
// Camera movement action
////////////////////////////////////////////////////////////

void SimulationsTransition::SetCamera(Camera *camera)
{
    m_Camera = camera;
}

void SimulationsTransition::TurnCamera(float angle, bool withLimit /*= true*/, float angularSpeed /*= 0.2f*/)
{
    if (!withLimit || m_Camera->getAngleHorizon() < angle)
    {
        m_Camera->moveLeft( angularSpeed * m_DeltaT);
    }
}

void SimulationsTransition::ZoomIn()
{
}

void SimulationsTransition::MoveIntoCloth()
{
    slmath::vec3 position = m_Camera->getPointToFollow();
    position.x -= m_SpeedAccelerator * m_DeltaT;
    m_Camera->setPointToFollow(position);
}

void SimulationsTransition::RotateCamera()
{
    if (m_Camera->getAngleHeight() > -slmath::PI / 4.0f)
    {
        m_Camera->moveUp(-0.08f * m_DeltaT);
    }
}

void SimulationsTransition::RotateCamera2()
{
    if (m_Camera->getAngleHeight() < slmath::PI / 32.0f)
    {
        m_Camera->moveUp(0.08f * m_DeltaT);
    }
}

void SimulationsTransition::Decelerate()
{
    m_SpeedAccelerator = 40.0f;
}

void SimulationsTransition::GoToForceField()
{
    const float newDistanceFromTarget = 50.0f;
    if (m_Camera->getDistanceFromTarget() < newDistanceFromTarget)
    {
        m_Camera->setDistanceFromTarget(m_Camera->getDistanceFromTarget() + slmath::min(5.0f * m_DeltaT, newDistanceFromTarget));
    }

    slmath::vec3 position = *reinterpret_cast<slmath::vec4*>(&m_AcceleratorForceField.m_Position);

    slmath::vec3 pointToFollow = m_Camera->getPointToFollow();

    slmath::vec3 difference = position - pointToFollow;
    float distance = length(difference);

    if (distance < 0.1f)
    {
        return;
    }
    difference = normalize(difference);
    const float speed = 50.0f;
    slmath::vec4 newPosition = pointToFollow + difference * slmath::min(speed * m_DeltaT, distance);
    m_Camera->setPointToFollow(newPosition);
    m_AcceleratorForceField4.m_Position = *reinterpret_cast<vrVec4*>(&newPosition);
    m_PhysicsParticle4.UpdateAccelerator(0, m_AcceleratorForceField4);
}

////////////////////////////////////////////////////////////
// Water Particles action - m_PhysicsParticle5
////////////////////////////////////////////////////////////

void SimulationsTransition::InitAnimationWater(size_t index)
{
    m_PhysicsParticle5.SetEnableGridOnGPU(false);
    m_PhysicsParticle5.SetEnableSPHAndIntegrateOnGPU(false);
    m_PhysicsParticle5.SetEnableCollisionOnGPU(false);
    m_PhysicsParticle5.SetEnableAcceleratorOnGPU(false);
    m_PhysicsParticle5.SetEnableAnimation(true);
    m_PhysicsParticle5.SetParticlesAnimation(reinterpret_cast<vrVec4*>(m_ParticleAnimation[index]));
    m_PhysicsParticle5.InitializeOpenClData();

    m_PhysicsParticle5.SetAnimationTime(0.0f);
    m_PhysicsParticle5.Simulate();
}

void SimulationsTransition::KillSpeedWater()
{
    m_PhysicsParticle5.SetEnableGridOnGPU(false);
    m_PhysicsParticle5.SetEnableSPHAndIntegrateOnGPU(false);
    m_PhysicsParticle5.SetEnableCollisionOnGPU(false);
    m_PhysicsParticle5.SetEnableAnimation(false);
    m_PhysicsParticle5.SetEnableAcceleratorOnGPU(true);
    m_AcceleratorWater5.m_Type = vrAccelerator::KILL_SPEED;
    m_PhysicsParticle5.UpdateAccelerator(0, m_AcceleratorWater5);
    m_PhysicsParticle5.Simulate();
}

void SimulationsTransition::InitWater()
{
    KillSpeedWater();
    m_PhysicsParticle5.SetEnableGridOnGPU(true);
    m_PhysicsParticle5.SetEnableSPHAndIntegrateOnGPU(true);
    m_PhysicsParticle5.SetEnableCollisionOnGPU(true);
    m_PhysicsParticle5.SetEnableAcceleratorOnGPU(false);
    m_PhysicsParticle5.SetEnableAnimation(false);
    m_PhysicsParticle5.CreateGrid();
}

void SimulationsTransition::MoveCameraToWater()
{
    const slmath::vec3 center(0.0f, -20.0f, 0.0f);
    slmath::vec3 direction = center - m_Camera->getPointToFollow();
    if (slmath::dot(direction, direction) > 2.0f)
    {
        direction = slmath::normalize(direction);
        m_Camera->setPointToFollow(m_Camera->getPointToFollow() + 30.0f * m_DeltaT * direction);
    }
    else
    {
        m_Camera->setPointToFollow(center);
    }
}

////////////////////////////////////////////////////////////
// Flying Particles action - m_PhysicsParticle4
////////////////////////////////////////////////////////////

void SimulationsTransition::GoToCenter()
{
    const float newDistanceFromTarget = 50.0f;
    if (m_Camera->getDistanceFromTarget() < newDistanceFromTarget)
    {
        m_Camera->setDistanceFromTarget(m_Camera->getDistanceFromTarget() + slmath::min(5.0f * m_DeltaT, newDistanceFromTarget));
    }

    slmath::vec3 position = *reinterpret_cast<slmath::vec4*>(&m_AcceleratorForceField.m_Position);

    slmath::vec3 pointToFollow = m_Camera->getPointToFollow();

    slmath::vec3 difference = position - pointToFollow;
    float distance = length(difference);

    if (distance < 0.1f)
    {
        return;
    }
    difference = normalize(difference);
    const float speed = 50.0f;
    slmath::vec4 newPosition = pointToFollow + difference * slmath::min(speed * m_DeltaT, distance);
    newPosition.y = m_AcceleratorForceField4.m_Position.y;
    m_Camera->setPointToFollow(newPosition);
    m_AcceleratorForceField4.m_Position = *reinterpret_cast<vrVec4*>(&newPosition);
    m_PhysicsParticle4.UpdateAccelerator(0, m_AcceleratorForceField4);

}

void SimulationsTransition::MoveAccelerator4()
{
    slmath::vec4 position = *reinterpret_cast<slmath::vec4*>(&m_AcceleratorForceField4.m_Position);
    if (position.z > 0.0f)
    {    
        position.z -= m_SpeedAccelerator * m_DeltaT;
        m_AcceleratorForceField4.m_Position = *reinterpret_cast<vrVec4*>(&position);
        m_PhysicsParticle4.UpdateAccelerator(0, m_AcceleratorForceField4);
    }
}

void SimulationsTransition::MoveAcceleratorDirection4()
{
    const slmath::vec3 direction(-1.0f, 0.0f, -1.0f);
    slmath::vec4 position = *reinterpret_cast<slmath::vec4*>(&m_AcceleratorForceField4.m_Position);
    m_VelocityAccelerator = direction * m_SpeedAccelerator;
    position += m_VelocityAccelerator * m_DeltaT;
    m_AcceleratorForceField4.m_Position = *reinterpret_cast<vrVec4*>(&position);
    m_PhysicsParticle4.UpdateAccelerator(0, m_AcceleratorForceField4);
}


void SimulationsTransition::RevoluteAccelerator4()
{
    slmath::vec4 position = *reinterpret_cast<slmath::vec4*>(&m_AcceleratorForceField4.m_Position);
    m_SpeedAccelerator = 40.0f;
    const float acceleration = 0.2f;
    const float randomAcceleration = 1.0f;
    slmath::vec3 direction = -position;
    direction = slmath::normalize(direction);

    direction += slmath::vec4(  rand() % 11 * randomAcceleration - 5.0f * randomAcceleration,
                                        0.0f,
                                        rand() % 11 * randomAcceleration - 5.0f * randomAcceleration);

    m_VelocityAccelerator += acceleration * direction;    
    position += m_VelocityAccelerator * m_DeltaT;
    m_AcceleratorForceField4.m_Position = *reinterpret_cast<vrVec4*>(&position);

    m_PhysicsParticle4.UpdateAccelerator(0, m_AcceleratorForceField4);
}


void SimulationsTransition::FollowParticles()
{
    vrVec3 position = m_AcceleratorForceField4.m_Position;
    m_Camera->setPointToFollow(*reinterpret_cast<slmath::vec3*>(&position));
}


void SimulationsTransition::MoveClothAcceleratorByParticles4()
{
    m_Accelerator.m_Position = m_AcceleratorForceField4.m_Position;
    m_PhysicsParticle.UpdateAccelerator(0, m_Accelerator);
}

void SimulationsTransition::MoveLight()
{
    m_LightPosition = *reinterpret_cast<slmath::vec3*>(&m_AcceleratorForceField4.m_Position);
}

void SimulationsTransition::MoveSpheres()
{
    vrSphere *spheres = m_PhysicsParticle.GetOutsideSpheres();
    for (int i = 1; i < m_SpheresCount; i++)
    {
        // Get physics sphere data
        vrSphere &sphere = spheres[i];
        // Update physics
        vrVec3 position = m_AcceleratorForceField4.m_Position;
        sphere.m_Position = position;
        // Update renderer
        float colorAndSize = m_Spheres[i].w;
        m_Spheres[i] = *reinterpret_cast<slmath::vec4*>(&sphere);
        m_Spheres[i].w = colorAndSize;
    }
}

void SimulationsTransition::MoveSphere4()
{
    vrSphere *spheres = m_PhysicsParticle4.GetInsideSpheres();
    spheres[0].m_Position = m_AcceleratorForceField4.m_Position;
}

void SimulationsTransition::MoveFlyingParticlesByForceField()
{
    const int index = (64 * 64 * 64 * 4) / 4;
    m_AcceleratorForceField4.m_Position = m_PhysicsParticle2.GetParticlePositions()[index];
    m_PhysicsParticle4.UpdateAccelerator(0, m_AcceleratorForceField4);
    MoveSphere4();
}

////////////////////////////////////////////////////////////
// Sky action - m_PhysicsParticle3
////////////////////////////////////////////////////////////

void SimulationsTransition::InitAnimationSky3()
{
    m_PhysicsParticle3.SetEnableAcceleratorOnGPU(false);
    m_PhysicsParticle3.SetEnableAnimation(true);
    m_PhysicsParticle3.SetAnimationTime(0.0f);
    m_PhysicsParticle3.Simulate();
}

////////////////////////////////////////////////////////////
// Force field action - m_PhysicsParticle2
////////////////////////////////////////////////////////////

void SimulationsTransition::MoveForceField()
{
    
    const float speedDirection = 10.0f;   
    slmath::vec3 direction = *reinterpret_cast<slmath::vec4*>(&m_AcceleratorForceField.m_Direction);
    direction += slmath::vec4(rand() % 11 * speedDirection - 5.0f*speedDirection,
                              rand() % 11 * speedDirection - 5.0f*speedDirection,
                              rand() % 11 * speedDirection - 5.0f*speedDirection);
    m_AcceleratorForceField.m_Direction = *reinterpret_cast<const vrVec4*>(&direction);


    const float acceleration = 1.0f;
    m_VelocityAcceleratorForceField += slmath::vec4(rand() % 11 * acceleration - 5.0f*acceleration,
                                          rand() % 11 * acceleration - 4.8f*acceleration, 
                                          rand() % 11 * acceleration - 5.0f*acceleration);

    slmath::vec3 position = *reinterpret_cast<slmath::vec4*>(&m_AcceleratorForceField.m_Position);
    const float damping = 0.99f;
    m_VelocityAcceleratorForceField *= damping;
    position += m_VelocityAcceleratorForceField * m_DeltaT;
    m_AcceleratorForceField.m_Position = *reinterpret_cast<const vrVec4*>(&position);

    m_Camera->setPointToFollow(position);
    
    m_PhysicsParticle2.UpdateAccelerator(0, m_AcceleratorForceField);
}

void SimulationsTransition::StartForceFieldCollision()
{
    const slmath::vec4 position(0.0f, 200.0f, 0.0f);
    m_AcceleratorForceField.m_Position = *reinterpret_cast<const vrVec4*>(&position);
    const slmath::vec4 direction(0.0f, -20.0f, 0.0f);
    m_AcceleratorForceField.m_Direction =*reinterpret_cast<const vrVec4*>(&direction);
    m_AcceleratorForceField.m_Type = vrAccelerator::SIMPLE_FORCE;
    m_PhysicsParticle2.UpdateAccelerator(0, m_AcceleratorForceField);

    m_PhysicsParticle2.SetEnableCollisionOnGPU(true);
}

void SimulationsTransition::StartForceField()
{
    m_PhysicsParticle2.SetEnableAcceleratorOnGPU(true);
    m_PhysicsParticle2.InitializeOpenClData();
    m_PhysicsParticle3.SetEnableAcceleratorOnGPU(true);
    m_PhysicsParticle3.InitializeOpenClData();
}

void SimulationsTransition::KillSpeedForceField()
{
    m_AcceleratorForceField.m_Type = vrAccelerator::KILL_SPEED;
    m_PhysicsParticle2.UpdateAccelerator(0, m_AcceleratorForceField);
    m_PhysicsParticle2.Simulate();
}

void SimulationsTransition::AttractForceField()
{
    m_AcceleratorForceField.m_Type = vrAccelerator::ATTARCTION;
    m_PhysicsParticle2.UpdateAccelerator(0, m_AcceleratorForceField);
}

void SimulationsTransition::MoveCameraToAttraction()
{
    const slmath::vec3 attraction(0.0f, 200.0f, 0.0f);
    slmath::vec3 direction = attraction - m_Camera->getPointToFollow();
    if (slmath::dot(direction, direction) > 2.0f)
    {
        direction = slmath::normalize(direction);
        m_Camera->setPointToFollow(m_Camera->getPointToFollow() + 100.0f * m_DeltaT * direction);
    }
    else
    {
        m_Camera->setPointToFollow(attraction);
    }
}

void SimulationsTransition::InitAnimationForceField2()
{
    m_PhysicsParticle2.SetEnableAcceleratorOnGPU(false);
    m_PhysicsParticle2.SetEnableCollisionOnGPU(false);
    m_PhysicsParticle2.SetEnableAnimation(true);
    m_PhysicsParticle2.SetAnimationTime(0.0f);
    m_PhysicsParticle2.Simulate();
}

void SimulationsTransition::RepulseForceField()
{
    m_PhysicsParticle2.SetEnableAcceleratorOnGPU(true);
    m_PhysicsParticle2.SetEnableCollisionOnGPU(true);
    m_PhysicsParticle2.SetEnableAnimation(false);

    const slmath::vec4 attraction(0.0f, 200.0f, 0.0f);
    m_AcceleratorForceField.m_Position = *reinterpret_cast<const vrVec4*>(&attraction);
    m_AcceleratorForceField.m_Type = vrAccelerator::REPULSION;
    m_PhysicsParticle2.UpdateAccelerator(0, m_AcceleratorForceField);
}

////////////////////////////////////////////////////////////
// Cloth action - m_PhysicsParticle
////////////////////////////////////////////////////////////


void SimulationsTransition::MoveSphereForCamera()
{
    vrSphere *spheres = m_PhysicsParticle.GetOutsideSpheres();
    // Get physics sphere data
    vrSphere &sphere = spheres[0];
    // Update physics
    slmath::vec3 cameraPosition = m_Camera->getPosition();
    sphere.m_Position = *reinterpret_cast<vrVec3*>(&cameraPosition );
    // Update renderer
    float colorAndSize = m_Spheres[0].w;
    m_Spheres[0] = *reinterpret_cast<slmath::vec4*>(&sphere);
    m_Spheres[0].w = colorAndSize;
}

void SimulationsTransition::StartWind()
{
    slmath::vec4 direction = *reinterpret_cast<slmath::vec4*>(&m_Accelerator.m_Direction);
    direction += slmath::vec4(rand()%11 * 0.1f - 0.5f, rand() % 11 * 0.1f - 0.7f, rand()%11 * 0.1f - 0.3f);
    direction = 10.0f * normalize(direction);
}

void SimulationsTransition::StrongtWind()
{
    slmath::vec4 direction = *reinterpret_cast<slmath::vec4*>(&m_Accelerator.m_Direction);
    direction = slmath::vec4(0.0f, 0.5f, 1.0f);
    direction = 50.0f * normalize(direction);
    m_Accelerator.m_Direction = *reinterpret_cast<const vrVec4*>(&direction);
 
    m_Accelerator.m_Type = vrAccelerator::SIMPLE_FORCE;
    m_PhysicsParticle.UpdateAccelerator(0, m_Accelerator);
}

void SimulationsTransition::DetachCloth(int indexCloth)
{
    int index = m_SideCloth * m_SideCloth * indexCloth;
    for (int i = 0; i < m_SideCloth; i++)
    {
        m_PhysicsParticle.GetParticlePositions()[index].w = 1.0f;
        index += m_SideCloth;
    }
}

void SimulationsTransition::DetachClothes()
{
    for (int k = 0; k < m_ClothCount; k++)
    {
        DetachCloth(k);
    }
    // Force to send data on the GPU
    m_PhysicsParticle.InitializeOpenClData();
}


////////////////////////////////////////////////////////////
// System particles initilization
////////////////////////////////////////////////////////////

void SimulationsTransition::CreateCollisionsForWater()
{
    vrAabb aabb;
    const slmath::vec4 boxMin(-500.0f, -20.0f, -500.0f);
    const slmath::vec4 boxMax(500.0f, 1000.0f, 500.0f);
    aabb.m_Min = *reinterpret_cast<const vrVec4*>(&boxMin);
    aabb.m_Max = *reinterpret_cast<const vrVec4*>(&boxMax);

    m_PhysicsParticle5.AddInsideAabb(aabb);
}

void SimulationsTransition::CreateWater()
{
    const int sideBox = 8;
    const int halfSideBox = sideBox / 2;
    const int length = 64;
    const float spaceBetweenParticles = 0.9f;

    m_Particles5Count = sideBox * sideBox * sideBox * length;
    m_StartPositions5 = new slmath::vec4[m_Particles5Count];

    const int animationCount = 4;

    std::string xmlFiles[] = {"Shell", "Chris2", "Elliot2", "Vincent2", "Bend"};
    for (int i = 0; i < animationCount; i++)
    {
        m_ParticleAnimation[i] = new slmath::vec4[m_Particles5Count];
        BaseDemo::FillPositionFromXml(  xmlFiles[i], std::string("noi:GEO_POINT"),  
                                        reinterpret_cast<slmath::vec4*>(m_ParticleAnimation[i]), m_Particles5Count);
    }
    

    slmath::vec4 color = slmath::vec4(0.3f, 0.2f, 0.1f, 2.0f);

    int sqrtParticles5Count = int(sqrt(float(m_Particles5Count)));
    const int spaceBetweenParticlesInit = 3;
    const float height = 2.0f;

    for (int i = 0; i < m_Particles5Count; i++)
    {
        const int noisePower = 1;
        int noiseX = (rand()%11 - 5) * noisePower;
        int noiseZ = (rand()%11 - 5) * noisePower;
        m_StartPositions5[i] = slmath::vec4(float((i % sqrtParticles5Count - sqrtParticles5Count / 2) * spaceBetweenParticlesInit + noiseX) ,
                                            height, 
                                            -float((i / sqrtParticles5Count - sqrtParticles5Count / 2) * spaceBetweenParticlesInit + noiseZ));
        m_StartPositions5[i].w = Compress(color);
    }


    int index = 0;
    for (int i = 0; i < sideBox; i++)
    {
        for (int j = 0; j < length*sideBox; j++)
        {
            for (int k = 0; k < sideBox; k++)
            {
                assert(index < m_Particles5Count);
                float noiseI = float((rand() % 100 - 50) * 0.0001f);
                float noiseJ = float((rand() % 100 - 50) * 0.0001f);
                float noiseK = float((rand() % 100 - 50) * 0.0001f);

                assert(index < m_Particles5Count);
                m_ParticleAnimation[0][index] = slmath::vec4( (i - halfSideBox + noiseI) * spaceBetweenParticles, 
                                                        (j + 5 + noiseJ) * spaceBetweenParticles, 
                                                        (k - halfSideBox + noiseK) * spaceBetweenParticles);


                for (int indexAnimation = 0; indexAnimation < animationCount; indexAnimation++)
                {
                    m_ParticleAnimation[indexAnimation][index].w = Compress(color);
                }
                index++;
            }
        }
    }


    CreateCollisionsForWater();
    m_PhysicsParticle5.SetParticlesAnimation(reinterpret_cast<vrVec4*>(m_ParticleAnimation[0]));

    m_PhysicsParticle5.SetIsUsingCPU(false);
    m_PhysicsParticle5.SetEnableGridOnGPU(true);
    m_PhysicsParticle5.SetEnableSPHAndIntegrateOnGPU(true);
    m_PhysicsParticle5.SetEnableSpringOnGPU(false);
    m_PhysicsParticle5.SetEnableCollisionOnGPU(true);
    m_PhysicsParticle5.SetEnableAcceleratorOnGPU(true);
    m_PhysicsParticle5.SetEnableAnimation(true);

    m_PhysicsParticle5.SetParticlesViscosity(5.0f);
    m_PhysicsParticle5.SetParticlesGazConstant(200.0f);
    m_PhysicsParticle5.SetParticlesMass(10.0f);
    slmath::vec4 gravity(0.0f, -100.0f, 0.0f);
    m_PhysicsParticle5.SetParticlesAcceleration(*reinterpret_cast<vrVec4*>(&gravity));

    
    // Add accelerator
    m_AcceleratorWater5.m_Type = vrAccelerator::KILL_SPEED;
    m_PhysicsParticle5.AddAccelerator(m_AcceleratorWater5);

    m_PhysicsParticle5.SetIsUsingInteroperability(false);
    m_PhysicsParticle5.Initialize(reinterpret_cast<vrVec4*>(m_StartPositions5), m_Particles5Count);
}


void SimulationsTransition::CreateCollisionsForFlyingParticles()
{
    vrSphere sphere;
    sphere.m_Position = m_AcceleratorForceField4.m_Position;
    sphere.m_Radius = 20.0f;

    m_PhysicsParticle4.AddInsideSphere(sphere);
}


void SimulationsTransition::CreateFlyingParticles()
{
    m_Particles4Count = 256;
    m_StartPositions4 = new slmath::vec4[m_Particles4Count];


    m_ParticlesColors4[0] = slmath::vec4(0.4f, 0.4f, 0.4f, 3.0f);
    m_ParticlesColors4[1] = slmath::vec4(0.4f, 0.2f, 0.2f, 3.0f);
    m_ParticlesColors4[2] = slmath::vec4(0.2f, 0.4f, 0.2f, 3.0f);
    m_ParticlesColors4[3] = slmath::vec4(0.2f, 0.2f, 0.4f, 3.0f);
    m_ParticlesColors4[4] = slmath::vec4(0.2f, 0.4f, 0.4f, 3.0f);
    m_ParticlesColors4[5] = slmath::vec4(0.4f, 0.2f, 0.4f, 3.0f);
    m_ParticlesColors4[6] = slmath::vec4(0.4f, 0.4f, 0.2f, 3.0f);
    m_ParticlesColors4[7] = slmath::vec4(0.1f, 0.1f, 0.4f, 10.0f);
    m_ParticlesColors4[8] = slmath::vec4(0.1f, 0.4f, 0.1f, 10.0f);
    m_ParticlesColors4[9] = slmath::vec4(0.4f, 0.1f, 0.1f, 10.0f);
    
    int sqrtParticles4Count = int(sqrt(float(m_Particles4Count)));
    const float spaceBetweenParticles = 10.0f;
    const float height = 10.0f;


    for (int i = 0; i < m_Particles4Count; i++)
    {
        const int noisePower = 1;
        int noiseX = (rand()%11 - 5) * noisePower;
        int noiseZ = (rand()%11 - 5) * noisePower;
        m_StartPositions4[i] = slmath::vec4(float((i % sqrtParticles4Count - sqrtParticles4Count / 2) * spaceBetweenParticles + noiseX) ,
                                            height, 
                                            -float((i / sqrtParticles4Count - sqrtParticles4Count / 2) * spaceBetweenParticles + noiseZ));
        m_StartPositions4[i] += slmath::vec4(m_InitialFlyingPosition);
        m_StartPositions4[i].w = Compress(m_ParticlesColors4[rand()%m_ParticlesClolor4Count]);
    }



    CreateCollisionsForFlyingParticles();

    m_PhysicsParticle4.SetIsUsingCPU(false);
    m_PhysicsParticle4.SetEnableGridOnGPU(false);
    m_PhysicsParticle4.SetEnableSPHAndIntegrateOnGPU(false);
    m_PhysicsParticle4.SetEnableSpringOnGPU(false);
    m_PhysicsParticle4.SetEnableCollisionOnGPU(true);
    m_PhysicsParticle4.SetEnableAcceleratorOnGPU(true);

    // Add accelerator force field
    const slmath::vec4 acceleratorPosition(m_InitialFlyingPosition);    
    m_AcceleratorForceField4.m_Position = *reinterpret_cast<const vrVec4*>(&acceleratorPosition);
    m_AcceleratorForceField4.m_Radius = 8000.0f;

    const slmath::vec4 acceleratorDirectionForceField(0.0f, 0.0f, 1.0f);
    m_AcceleratorForceField4.m_Direction = *reinterpret_cast<const vrVec4*>(&acceleratorDirectionForceField);
    m_AcceleratorForceField4.m_Type = vrAccelerator::FORCE_FIELD;
    m_PhysicsParticle4.AddAccelerator(m_AcceleratorForceField4);

    m_PhysicsParticle4.Initialize(reinterpret_cast<vrVec4*>(m_StartPositions4), m_Particles4Count);
}


void SimulationsTransition::CreateSky()
{
    m_Particles3Count = 64 * 64 * 16;
    m_StartPositions3 = new slmath::vec4[m_Particles3Count];


    m_ParticlesColors3[0] = slmath::vec4(0.0f, 0.0f, 0.4f, 50.0f);
    m_ParticlesColors3[1] = slmath::vec4(0.0f, 0.0f, 0.4f, 50.0f);
    m_ParticlesColors3[2] = slmath::vec4(0.0f, 0.0f, 0.4f, 50.0f);
    m_ParticlesColors3[3] = slmath::vec4(0.0f, 0.0f, 0.4f, 50.0f);
    m_ParticlesColors3[4] = slmath::vec4(0.0f, 0.0f, 0.4f, 50.0f);
    m_ParticlesColors3[5] = slmath::vec4(0.0f, 0.0f, 0.4f, 50.0f);
    m_ParticlesColors3[6] = slmath::vec4(0.4f, 0.4f, 0.4f, 50.0f);
    m_ParticlesColors3[7] = slmath::vec4(0.2f, 0.0f, 0.0f, 50.0f);
    m_ParticlesColors3[8] = slmath::vec4(0.3f, 0.1f, 0.0f, 50.0f);
    m_ParticlesColors3[9] = slmath::vec4(0.2f, 0.2f, 0.0f, 50.0f);

    
    int sqrtParticles3Count = int(sqrt(float(m_Particles3Count)));
    const int spaceBetweenParticles = 20;
    const float height = 300.0f;

    for (int i = 0; i < m_Particles3Count; i++)
    {
        const int noisePower = 200;
        int noiseX = (rand()%11 - 5) * noisePower;
        int noiseZ = (rand()%11 - 5) * noisePower;
        float x = float((i % sqrtParticles3Count - sqrtParticles3Count / 2) * spaceBetweenParticles + noiseX);
        float z = -float((i / sqrtParticles3Count - sqrtParticles3Count / 2) * spaceBetweenParticles + noiseZ);
        m_StartPositions3[i] = slmath::vec4( x, slmath::clamp(height +  ( - abs(x) - abs(z) ) * 0.1f, -30.0f, height) , z);
        m_StartPositions3[i].w = Compress(m_ParticlesColors3[rand()%m_ParticlesClolor3Count]);
    }


    m_PhysicsParticle3.SetIsUsingCPU(false);
    m_PhysicsParticle3.SetEnableGridOnGPU(false);
    m_PhysicsParticle3.SetEnableSPHAndIntegrateOnGPU(false);
    m_PhysicsParticle3.SetEnableSpringOnGPU(false);
    m_PhysicsParticle3.SetEnableCollisionOnGPU(false);
    m_PhysicsParticle3.SetEnableAcceleratorOnGPU(true);
    m_PhysicsParticle3.SetEnableAnimation(true);

    m_PhysicsParticle3.SetParticlesAnimation(reinterpret_cast<vrVec4*>(m_StartPositions3));

    // Add accelerator force field
    for (int i = 0; i < 1; i++)
    {
        const slmath::vec4 acceleratorPosition((i/2)*2000.0f /*- 1000.0f*/, 200.0f, (i%2)*2000.0f /*- 1000.0f*/);
        m_AcceleratorForceField3.m_Position = *reinterpret_cast<const vrVec4*>(&acceleratorPosition);
        m_AcceleratorForceField3.m_Radius = 10000.0f;

        const slmath::vec4 acceleratorDirectionForceField(0.0f, 1.0f, 0.0f);
        m_AcceleratorForceField3.m_Direction = *reinterpret_cast<const vrVec4*>(&acceleratorDirectionForceField);
        m_AcceleratorForceField3.m_Type = vrAccelerator::CIRCULAR_FORCE;
        m_PhysicsParticle3.AddAccelerator(m_AcceleratorForceField3);
    }

    m_PhysicsParticle3.SetIsUsingInteroperability(false);
    m_PhysicsParticle3.Initialize(reinterpret_cast<vrVec4*>(m_StartPositions3), m_Particles3Count);
}

void SimulationsTransition::CreateCollisionsForForceField()
{
    vrAabb aabb;
    const slmath::vec4 boxMin(-100000.0f, 200.0f, -100000.0f);
    const slmath::vec4 boxMax(100000.0f, 100000.0f, 100000.0f);
    aabb.m_Min = *reinterpret_cast<const vrVec4*>(&boxMin);
    aabb.m_Max = *reinterpret_cast<const vrVec4*>(&boxMax);

    m_PhysicsParticle2.AddInsideAabb(aabb);

    vrSphere sphere;
    slmath::vec3 spherePosition(1000.0f, -1000.0f, 200.0f);
    sphere.m_Position = *reinterpret_cast<const vrVec4*>(&spherePosition);
    sphere.m_Radius = 0.1f;

    m_PhysicsParticle2.AddOutsideSphere(sphere);
}

void SimulationsTransition::ForceFieldColor(int index)
{
    int randomNumber = rand() % 50;
    if (randomNumber == 0)
    {
        if (rand() % 10 == 0)
        {
            m_StartPositions2[index].w = Compress(m_ParticlesColors[1]);
        }
        else
        {
            m_StartPositions2[index].w = Compress(m_ParticlesColors[0]);
        }
    }
    else
    {
        m_StartPositions2[index].w = Compress(m_ParticlesColors[randomNumber % (m_ParticlesClolorCount - 2)  + 2 ]);
    }
}

void SimulationsTransition::CreateForceField()
{
    m_Particles2Count = 64 * 64 * 64 * 4;
    const float incrementX = 0.5f;

    m_StartPositions2 = new slmath::vec4[m_Particles2Count];

  
    m_ParticlesColors[0] = slmath::vec4(0.0f, 0.4f, 0.0f, 50.0f);
    m_ParticlesColors[1] = slmath::vec4(0.4f, 0.4f, 0.0f, 50.0f);
    m_ParticlesColors[2] = slmath::vec4(1.0f, 1.0f, 1.0f, 3.0f);
    m_ParticlesColors[3] = slmath::vec4(0.2f, 0.6f, 0.2f, 3.0f);
    m_ParticlesColors[4] = slmath::vec4(0.6f, 0.6f, 0.2f, 3.0f);
    m_ParticlesColors[5] = slmath::vec4(0.2f, 0.6f, 0.2f, 3.0f);
    m_ParticlesColors[6] = slmath::vec4(0.6f, 0.6f, 0.2f, 3.0f);
    m_ParticlesColors[7] = slmath::vec4(0.2f, 0.6f, 0.2f, 3.0f);
    m_ParticlesColors[8] = slmath::vec4(0.2f, 0.6f, 0.2f, 3.0f);
    m_ParticlesColors[9] = slmath::vec4(0.2f, 0.6f, 0.2f, 3.0f);

    

    m_Particles2ForClothLines = 0; // 64 * 64 * 2;
    

    int particlesFloorCount = m_Particles2Count - m_Particles2ForClothLines;
    
    int particlesForHorizontalLinesCount = particlesFloorCount / 2;

    int linesCount = 72;

    int indexSideCount = particlesForHorizontalLinesCount / linesCount;

    float spaceBetweenLines = 50.0f;

    for (int j = 0; j < linesCount; j++)
    {
        for (int i = 0; i < indexSideCount; i++)
        {
            int index = m_Particles2ForClothLines + i+j*indexSideCount;
            m_StartPositions2[index] = slmath::vec4(-indexSideCount / 2.0f * incrementX + incrementX * i,
                                                    -20.0f, 
                                                    -linesCount / 2.0f * spaceBetweenLines +spaceBetweenLines * j);
            

            ForceFieldColor(index);
        }
    }

    for (int j = 0; j < linesCount; j++)
    {
        for (int i = 0; i < indexSideCount; i++)
        {
            int index = m_Particles2ForClothLines + linesCount * indexSideCount + i+j*indexSideCount ;
            m_StartPositions2[index] = slmath::vec4(linesCount / 2.0f * spaceBetweenLines - spaceBetweenLines * j,
                                                    -20.0f, 
                                                    -indexSideCount / 2.0f * incrementX + incrementX * i);
            ForceFieldColor(index);

        }
    }


    CreateCollisionsForForceField();


    m_PhysicsParticle2.SetIsUsingCPU(false);

    m_PhysicsParticle2.SetEnableGridOnGPU(false);
    m_PhysicsParticle2.SetEnableSPHAndIntegrateOnGPU(false);
    m_PhysicsParticle2.SetEnableSpringOnGPU(false);
    m_PhysicsParticle2.SetEnableCollisionOnGPU(true);
    m_PhysicsParticle2.SetEnableAcceleratorOnGPU(true);
    m_PhysicsParticle2.SetEnableAnimation(true);

    m_PhysicsParticle2.SetParticlesAnimation(reinterpret_cast<vrVec4*>(m_StartPositions2));

    // Add accelerator force field
    const slmath::vec4 acceleratorPosition(0.0f, -50.0f, 0.0f);    
    m_AcceleratorForceField.m_Position = *reinterpret_cast<const vrVec4*>(&acceleratorPosition);
    m_AcceleratorForceField.m_Radius = 10000.0f;

    const slmath::vec4 acceleratorDirectionForceField(1.0f, 1.0f, 0.0f);
    m_AcceleratorForceField.m_Direction = *reinterpret_cast<const vrVec4*>(&acceleratorDirectionForceField);
    m_AcceleratorForceField.m_Type = vrAccelerator::FORCE_FIELD;

    m_PhysicsParticle2.AddAccelerator(m_AcceleratorForceField);
    m_VelocityAcceleratorForceField = slmath::vec3(0.0f, 0.0f, 0.0f);

    
    m_PhysicsParticle2.SetIsUsingInteroperability(false);
    m_PhysicsParticle2.Initialize(reinterpret_cast<vrVec4*>(m_StartPositions2), m_Particles2Count);
}

void SimulationsTransition::CreateCollisionsForCloth()
{
// Add limitation

    const slmath::vec4 boxMin(-10000.0f, -20.0f, -10000.0f);
    const slmath::vec4 boxMax(10000.0f, 10000.0f, 10000.0f);

    vrAabb aabb;

    aabb.m_Min = *reinterpret_cast<const vrVec4*>(&boxMin);
    aabb.m_Max = *reinterpret_cast<const vrVec4*>(&boxMax);

    m_PhysicsParticle.AddInsideAabb(aabb);



    const float spaceBetweenSphere = 100.0f;
    m_SpheresCount = 2;
    m_Spheres = new slmath::vec4[25];
    int sphereIndex = 0;
    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 5; j++)
        {    
            m_SpheresCount++;
            vrSphere sphere;

            slmath::vec3 spherePosition(0.0f, 20.0f, -100.0f -m_ClothCount / 2.0f * m_SpaceBetweenCloth - m_SpheresCount * spaceBetweenSphere );
            sphere.m_Position = *reinterpret_cast<const vrVec4*>(&spherePosition);
            sphere.m_Radius = 2.0f;

            m_Spheres[sphereIndex] = *reinterpret_cast<slmath::vec4*>(&spherePosition);
            m_Spheres[sphereIndex].w = Compress(slmath::vec4(1.0f, 1.0f, 1.0f, sphere.m_Radius - 1.0f));

            /*if (sphereIndex == 1)
            {
                slmath::vec3 spherePosition(0.0f, 0.0f, 0.0f);
                sphere.m_Position = *reinterpret_cast<const vrVec4*>(&spherePosition);
                sphere.m_Radius = 50.0f;

                m_Spheres[sphereIndex] = *reinterpret_cast<slmath::vec4*>(&spherePosition);
                m_Spheres[sphereIndex].w = Compress(slmath::vec4(0.1f, 0.2f, 1.0f, sphere.m_Radius - 1.0f));
            }*/

            m_PhysicsParticle.AddOutsideSphere(sphere);


            sphereIndex++;

        }
    }
}

void SimulationsTransition::CreateCloth()
{
    m_SideCloth = 16;
    m_ClothCount = 1024;
    m_ClothActivatedCount = m_ClothCount;
    m_SpaceBetweenCloth = 50.0f;

    const int sideClothMinusOne = m_SideCloth - 1;
    const int halfSideCloth = m_SideCloth / 2;
    m_ParticlesCount = m_SideCloth * m_SideCloth * m_ClothCount;
    const float spaceBetweenParticles = 1.5f;
    m_StartPositions = new slmath::vec4[m_ParticlesCount];
    int index = 0;


    m_Indirections = new int[m_ClothCount];


    for (int i = 0; i < m_ClothCount; i++)
    {
        m_Indirections[i] = i;
    }

    vrSpring nullSpring;

    nullSpring.m_Distance = 0.0f;
    nullSpring.m_ParticleIndex1 = 0;
    nullSpring.m_ParticleIndex2 = 0;

    int sqrtClothCount = int(sqrt(float(m_ClothCount)));

    for (int k = 0; k < m_ClothCount; k++)
    {
        /*slmath::vec4 clothPosition(0.0f,
                                    20.0f,
                                    -m_ClothCount / 2.0f * m_SpaceBetweenCloth + (float)k * m_SpaceBetweenCloth);*/
        const float spaceRandom = 0.1f;
        const float spaceRandomY = 0.1f;

        float noiseClothPositionX = float((rand() % 11 - 5) * spaceRandom);
        float noiseClothPositionY = float((rand() % 11 - 2) * spaceRandomY);
        float noiseClothPositionZ = float((rand() % 11 - 5) * spaceRandom);


         slmath::vec4 clothPosition( ( (k % sqrtClothCount - sqrtClothCount / 2 ) + noiseClothPositionX) * m_SpaceBetweenCloth,
                                    noiseClothPositionY * m_SpaceBetweenCloth,
                                    ( (k / sqrtClothCount - sqrtClothCount / 2 ) + noiseClothPositionZ) * m_SpaceBetweenCloth);
        
        for (int i = 0; i < m_SideCloth; i++)
        {
            for (int j = 0; j < m_SideCloth; j++)
            {
                assert(index < m_ParticlesCount);   
                float noise = 0.0f;
                slmath::vec4 particlePosition(float(i - halfSideCloth) * spaceBetweenParticles + noise,
                                              -float(j - halfSideCloth) * spaceBetweenParticles + noise,
                                              0.0f);

                m_StartPositions[index] = clothPosition + particlePosition;

                m_StartPositions[index].w = Compress(slmath::vec4(0.9f, 0.1f, 0.1f, 2.0f));


                
                if (j == 0)
                {
                    m_StartPositions[index].w = 0.0f;
                }
                else
                {
                    m_StartPositions[index].w = 1.0f;
                }
                index++;
            }
        }
    }

    // For structural constraints only
    int onePointOverTwoIndex = 0;
    // For structural and shearing constraints
    int oneLineOverTwoIndex = 0;
    // For bending constraints
    int onePointOverFourIndex = 0;
    int oneRowOverFourIndex = 0;
    

    const int particlesByCloth = m_ParticlesCount / m_ClothCount;
    const int halfParticlesByClothCount = particlesByCloth / 2;

    for (int k = 0; k < m_ClothCount; k++)
    {
        int firstParticleIndex = k * particlesByCloth;
        int lastParticleIndex = firstParticleIndex + particlesByCloth;
        for (int i = 0; i < halfParticlesByClothCount; i++)
        {
                
            // Structural constraints horizontal
            const int leftNeighborIndex = onePointOverTwoIndex - 1;
            if (leftNeighborIndex >= firstParticleIndex && leftNeighborIndex % m_SideCloth != sideClothMinusOne)
            {
                CreateSpring(onePointOverTwoIndex, leftNeighborIndex);
            }
            else
            {
                m_PhysicsParticle.AddSpring(nullSpring);
            }


            const int rightNeighborIndex = onePointOverTwoIndex + 1;
            if (rightNeighborIndex < lastParticleIndex && rightNeighborIndex % m_SideCloth != 0)
            {
                CreateSpring(onePointOverTwoIndex, rightNeighborIndex);
            }
            else
            {
                m_PhysicsParticle.AddSpring(nullSpring);
            }
                

            // Structural constraints vertical
            const int upNeighborIndex = oneLineOverTwoIndex - m_SideCloth;
            if (upNeighborIndex >= firstParticleIndex)
            {
                CreateSpring(oneLineOverTwoIndex, upNeighborIndex);
            }
            else
            {
                m_PhysicsParticle.AddSpring(nullSpring);
            }

            const int downNeighborIndex = oneLineOverTwoIndex + m_SideCloth;
            if (downNeighborIndex < lastParticleIndex)
            {
                CreateSpring(oneLineOverTwoIndex, downNeighborIndex);
            }
            else
            {
                m_PhysicsParticle.AddSpring(nullSpring);
            }

            // Shearing constraints
            // On the left

            const int leftUpNeighborIndex = upNeighborIndex - 1;
            if (leftUpNeighborIndex >= firstParticleIndex && leftUpNeighborIndex % m_SideCloth != sideClothMinusOne)
            {
                CreateSpring(oneLineOverTwoIndex, leftUpNeighborIndex);
            }
            else
            {
                m_PhysicsParticle.AddSpring(nullSpring);
            }

            const int leftDownNeighborIndex = downNeighborIndex - 1;
            if (leftDownNeighborIndex < lastParticleIndex && leftDownNeighborIndex % m_SideCloth != sideClothMinusOne)
            {
                CreateSpring(oneLineOverTwoIndex, leftDownNeighborIndex);
            }
            else
            {
                m_PhysicsParticle.AddSpring(nullSpring);
            }

            // Shearing on the right
            const int rightUpNeighborIndex = upNeighborIndex + 1;
            if (rightUpNeighborIndex >= firstParticleIndex && rightUpNeighborIndex % m_SideCloth != 0)
            {
                CreateSpring(oneLineOverTwoIndex, rightUpNeighborIndex);
            }
            else
            {
                m_PhysicsParticle.AddSpring(nullSpring);
            }

            const int rightDownNeighborIndex = downNeighborIndex + 1;
            if (rightDownNeighborIndex < lastParticleIndex && rightDownNeighborIndex % m_SideCloth != 0)
            {
                CreateSpring(oneLineOverTwoIndex, rightDownNeighborIndex);
            }
            else
            {
                m_PhysicsParticle.AddSpring(nullSpring);
            }

            // Increment for structural and shearing
            onePointOverTwoIndex += 2;

            if (i % m_SideCloth == sideClothMinusOne)
            {
                oneLineOverTwoIndex += m_SideCloth + 1;
            }
            else
            {
                oneLineOverTwoIndex++;
            }
            

           // Bending constraints horizontal (from left to right)
            const int right2NeighborIndex = onePointOverFourIndex + 2;
            if (right2NeighborIndex < lastParticleIndex && 
                onePointOverFourIndex / m_SideCloth == right2NeighborIndex / m_SideCloth) // same row
            {
                CreateSpring(onePointOverFourIndex, right2NeighborIndex);
            }
            else
            {
                m_PhysicsParticle.AddSpring(nullSpring);
            }

            const int right4NeighborIndex = right2NeighborIndex + 2;
            if (right4NeighborIndex < lastParticleIndex && 
                right2NeighborIndex / m_SideCloth == right4NeighborIndex / m_SideCloth)
            {
                CreateSpring(right2NeighborIndex, right4NeighborIndex);
            }
            else
            {
                m_PhysicsParticle.AddSpring(nullSpring);
            }

            // Bending from top to bottom
            const int down2NeighborIndex = oneRowOverFourIndex + 2 * m_SideCloth;
            if (right2NeighborIndex < lastParticleIndex && 
                oneRowOverFourIndex % m_SideCloth == down2NeighborIndex % m_SideCloth) // same column
            {
                CreateSpring(oneRowOverFourIndex, down2NeighborIndex);
            }
            else
            {
                m_PhysicsParticle.AddSpring(nullSpring);
            }

            const int down4NeighborIndex = down2NeighborIndex + 2 * m_SideCloth;
            if (down4NeighborIndex < lastParticleIndex && 
                down2NeighborIndex % m_SideCloth == down4NeighborIndex % m_SideCloth) // same column
            {
                CreateSpring(down2NeighborIndex, down4NeighborIndex);
            }
            else
            {
                m_PhysicsParticle.AddSpring(nullSpring);
            }

            // Increment for bending
            if (i % 2 == 0)
            {
                onePointOverFourIndex++;
            }
            else
            {
                onePointOverFourIndex += 3;
            }

            if (i % (2 * m_SideCloth) != 2 * m_SideCloth - 1)
            {
                oneRowOverFourIndex++;
            }
            else
            {
                oneRowOverFourIndex += 2 * m_SideCloth + 1;
            }
        }
    }

    CreateCollisionsForCloth();

    m_PhysicsParticle.SetEnableGridOnGPU(false);
    m_PhysicsParticle.SetEnableSPHAndIntegrateOnGPU(false);
    m_PhysicsParticle.SetEnableSpringOnGPU(true);
    m_PhysicsParticle.SetEnableCollisionOnGPU(false);
    m_PhysicsParticle.SetClothCount(m_ClothCount);
    m_PhysicsParticle.SetEnableAcceleratorOnGPU(true);
    m_PhysicsParticle.SetIsUsingCPU(false);


    
    // Accelerator 
    m_Accelerator.m_Type = vrAccelerator::REPULSION;
    slmath::vec4 cameraPosition = m_Camera->getPosition();
    m_Accelerator.m_Position = *reinterpret_cast<vrVec4*>(&cameraPosition );
    m_Accelerator.m_Radius = 3000.0f;
    m_PhysicsParticle.AddAccelerator(m_Accelerator);

    // Gravity
    vrAccelerator accelerator;
    const slmath::vec4 acceleratorDirection(0.0f, -20.0f, 0.0f);
    accelerator.m_Direction = *reinterpret_cast<const vrVec4*>(&acceleratorDirection);
    accelerator.m_Type = vrAccelerator::SIMPLE_FORCE;
    m_PhysicsParticle.AddAccelerator(accelerator);

    
    m_PhysicsParticle.Initialize(reinterpret_cast<vrVec4*>(m_StartPositions), m_ParticlesCount);
    delete[] m_StartPositions;
}

void SimulationsTransition::CreateSpring(int index1, int index2)
{
    vrSpring spring;
    spring.m_ParticleIndex1 = index1;
    spring.m_ParticleIndex2 = index2;
    slmath::vec4 positionIndex1 = m_StartPositions[index1];
    positionIndex1.w = 0.0f;
    slmath::vec4 positionIndex2 = m_StartPositions[index2];
    positionIndex2.w = 0.0f;
    float distance = slmath::length(positionIndex1 - positionIndex2);
    spring.m_Distance = distance;
    m_PhysicsParticle.AddSpring(spring);
}