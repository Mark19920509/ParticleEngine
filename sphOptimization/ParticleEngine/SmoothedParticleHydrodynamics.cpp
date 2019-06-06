
#include "SmoothedParticleHydrodynamics.h"
#include "Grid3D.h"

#include <slmath/slmath.h>
#include <cmath>
#include "Utility/Timer.h"
#include "ParticlesGPU/ParticlesGPU.hpp"
 

using namespace slmath;


SmoothedParticleHydrodynamics::SmoothedParticleHydrodynamics(Grid3D *grid3D) : m_Grid3D(grid3D),
                                                                m_ParticlesCount(0),
                                                                m_ParticlePositions(NULL),
                                                                m_Pressure(NULL),
                                                                m_Density(NULL),
                                                                m_PreviousDensity(NULL)
{
}

    

SmoothedParticleHydrodynamics::~SmoothedParticleHydrodynamics()
{
  
    if(m_Pressure)
        delete[] m_Pressure;
    if(m_Density)
        delete[] m_Density;
    if(m_PreviousDensity)
        delete[] m_PreviousDensity;
        
}

void SmoothedParticleHydrodynamics::Initialize(slmath::vec4 *positions, int positionsCount)
{
    m_ParticlePositions = positions;
     
    Timer::GetInstance()->StartTimerProfile();
    if (m_ParticlesCount != positionsCount)
    {
        ReallocParticles(positionsCount);
    }
    Timer::GetInstance()->StopTimerProfile("SPH: Allocate");
    Timer::GetInstance()->StartTimerProfile();
    for (int i = 0; i < positionsCount; i++)
    {
        m_PreviousDensity[i] = 0.0f;
        m_Density[i] = 0.0f;
        m_Pressure[i] = slmath::vec4(0.0f);
    }
    Timer::GetInstance()->StopTimerProfile("SPH: Set to 0");
    Timer::GetInstance()->StartTimerProfile();
    InitDensity();
    Timer::GetInstance()->StopTimerProfile("SPH: Init density");
}

void SmoothedParticleHydrodynamics::ReallocParticles(int particlesCount)
{
    delete[] m_Pressure;
    delete[] m_Density;
    delete[] m_PreviousDensity;

    m_ParticlesCount = particlesCount;
    m_Pressure = new slmath::vec4[m_ParticlesCount];
    m_Density = new float[m_ParticlesCount];
    m_PreviousDensity = new float[m_ParticlesCount];
}

void SmoothedParticleHydrodynamics::SetParticlesMass(float mass)
{
    m_SphParameters.m_Mass= mass;
}

void SmoothedParticleHydrodynamics::SetParticlesGazConstant(float gazConstant)
{
    m_SphParameters.m_GazConstant = gazConstant;
}

void SmoothedParticleHydrodynamics::SetParticlesMuViscosityt(float muViscosity)
{
    m_SphParameters.m_MuViscosity = muViscosity;
}

const SphParameters& SmoothedParticleHydrodynamics::GetParameters() const
{
    return m_SphParameters;
}

int SmoothedParticleHydrodynamics::GetParticlesCount() const
{
    return m_ParticlesCount;
}

slmath::vec4 *SmoothedParticleHydrodynamics::GetParticleAccelerations() const
{
    return m_Pressure;
}

float *SmoothedParticleHydrodynamics::GetDensity() const
{
    return m_Density;
}

float *SmoothedParticleHydrodynamics::GetPreviousDensity() const
{
    return m_PreviousDensity;
}

void SmoothedParticleHydrodynamics::Simulate(slmath::vec4 *positions, int positionsCount)
{
    assert(m_ParticlesCount == positionsCount);

    
    m_ParticlePositions = positions;

    Timer::GetInstance()->StartTimerProfile();

    ComputePressureQuery();
    SwapDensityBuffer(); 
    
    Timer::GetInstance()->StopTimerProfile("Compute pressure");

    for (int i = 0; i < m_ParticlesCount; i++)
    {
        assert(slmath::check(m_Pressure[i]));
        assert(slmath::check(m_PreviousDensity[i]));
    }
}

void SmoothedParticleHydrodynamics::InitDensity()
{
    for (int i = 0; i < m_ParticlesCount; i++)
    {
        m_PreviousDensity[i] = 0.0f;
        slmath::vec4 separation(0.0f);

        float distance = length(separation);
        m_PreviousDensity[i] += m_SphParameters.m_Mass * ( 15 / (PI*m_SphParameters.m_H*m_SphParameters.m_H*m_SphParameters.m_H) ) * ((1 - distance / m_SphParameters.m_H)*(1 - distance / m_SphParameters.m_H)*(1 - distance / m_SphParameters.m_H));

        assert(m_PreviousDensity[i] != 0.0f);
    }
}

void SmoothedParticleHydrodynamics::ComputePressure()
{
    for (int i = 0; i < m_ParticlesCount; i++)
    { 
        m_Pressure[i] = slmath::vec4(0.0f);
        m_Density[i] = 0.0f;
        for (int j = 0; j < m_ParticlesCount; j++)
        {
            assert(m_PreviousDensity[j] != 0.0f);
             slmath::vec4 separation = m_ParticlePositions[i] - m_ParticlePositions[j];
             float sqrDistance = dot(separation, separation);
            
            if (sqrDistance < m_SphParameters.m_H*m_SphParameters.m_H)
            {
                slmath::vec4 normal = separation;
                float distance = sqrt(sqrDistance);
                if (distance > 0.0f)
                {
                    normal /= distance;
                }
                m_Density[i] += m_SphParameters.m_Mass * ( 15 / (PI*m_SphParameters.m_H*m_SphParameters.m_H*m_SphParameters.m_H) ) * ((1 - distance / m_SphParameters.m_H)*(1 - distance / m_SphParameters.m_H)*(1 - distance / m_SphParameters.m_H));
                m_Pressure[i] += m_SphParameters.m_Mass / m_PreviousDensity[j] * 
                            ((m_SphParameters.m_GazConstant * m_PreviousDensity[i] + m_SphParameters.m_GazConstant * m_PreviousDensity[j]) * 0.5f) * 
                            ( 45 / (PI*m_SphParameters.m_H*m_SphParameters.m_H*m_SphParameters.m_H*m_SphParameters.m_H) ) * ((1 - distance / m_SphParameters.m_H)*(1 - distance / m_SphParameters.m_H)*(1 - distance / m_SphParameters.m_H)) * normal;
            }
        }
       m_Pressure[i] =  (m_SphParameters.m_Mass / m_PreviousDensity[i]) * m_Pressure[i];
    }
}

void SmoothedParticleHydrodynamics::ComputePressureBigRange()
{
    int average = 0;
    for (int i = 0; i < m_ParticlesCount; i++)
    { 

        int minNeighbor = m_Grid3D->GetNeighborsMin(i);
        int maxNeighbor = m_Grid3D->GetNeighborsMax(i);

        int indexParticle = m_Grid3D->GetParticleCellOrder()[i].m_ParticleIndex;

        int indexCell = 0;
        int cellPositions[27];
        m_Grid3D->GetNeighborsPositionIndex(i, cellPositions);

        m_Pressure[indexParticle] = slmath::vec4(0.0f);
        m_Density[indexParticle] = 0.0f;

        average += maxNeighbor - minNeighbor + 1;
        for (int j = minNeighbor; j <= maxNeighbor; j++)
        {

            assert (indexCell < 27);
             while (cellPositions[indexCell] < m_Grid3D->GetParticleCellOrder()[j].m_CellIndex)
            {
                indexCell++;
            }

            if (cellPositions[indexCell] > m_Grid3D->GetParticleCellOrder()[j].m_CellIndex)
                continue;

            int indexParticleNeighbor = m_Grid3D->GetParticleCellOrder()[j].m_ParticleIndex;
            
            assert(m_PreviousDensity[indexParticleNeighbor] != 0.0f);
             slmath::vec4 separation = m_ParticlePositions[indexParticle] - m_ParticlePositions[indexParticleNeighbor];
             float sqrDistance = dot(separation, separation);
            
            if (sqrDistance < m_SphParameters.m_H*m_SphParameters.m_H)
            {
                slmath::vec4 normal = separation;
                float distance = sqrt(sqrDistance);
                if (distance > 0.0f)
                {
                    normal /= distance;
                }
                m_Density[indexParticle] += m_SphParameters.m_Mass * ( 15 / (PI*m_SphParameters.m_H*m_SphParameters.m_H*m_SphParameters.m_H) ) * ((1 - distance / m_SphParameters.m_H)*(1 - distance / m_SphParameters.m_H)*(1 - distance / m_SphParameters.m_H));
                m_Pressure[indexParticle] += m_SphParameters.m_Mass / m_PreviousDensity[indexParticleNeighbor] * 
                            ((m_SphParameters.m_GazConstant * m_PreviousDensity[indexParticle] + m_SphParameters.m_GazConstant * m_PreviousDensity[indexParticleNeighbor]) * 0.5f) * 
                            ( 45 / (PI*m_SphParameters.m_H*m_SphParameters.m_H*m_SphParameters.m_H*m_SphParameters.m_H) ) * ((1 - distance / m_SphParameters.m_H)*(1 - distance / m_SphParameters.m_H)*(1 - distance / m_SphParameters.m_H)) * normal;
            }
        }
       m_Pressure[indexParticle] =  (m_SphParameters.m_Mass / m_PreviousDensity[indexParticle]) * m_Pressure[indexParticle];
       assert(m_Density[indexParticle] > 0.0f);
    }
    DEBUG_OUT("Average: \t" << average / m_ParticlesCount << "\n");
}


void SmoothedParticleHydrodynamics::ComputePressureQuery()
{
    const float sqrMaxDistance = length(slmath::vec4(2.0f)) * length(slmath::vec4(2.0f));
    UNREFERENCED_PARAMETER(sqrMaxDistance);
    int average = 0;
    int neighborsBuffer[1024];

    Grid3D::ParticleCellOrder *particleOrder = m_Grid3D->GetParticleCellOrder();
    
    for (int i = 0; i < m_ParticlesCount; i++)
    {

        int totalNeighborsCount = 0;

        // int neighborsCount = m_Grid3D->ComputeHashNeighbors(i,  neighborsBuffer, 256);
       //  int neighborsCount = m_Grid3D->GetNeighborsByParticleOrderHeuristic(i,  neighborsBuffer, 256);
       //int neighborsCount = m_Grid3D->GetNeighborsByParticleOrder(i,  neighborsBuffer, 256);
       // int neighborsCount = m_Grid3D->GetNeighborsByParticleOrderFullGrid(i,  neighborsBuffer, 1024);
        int neighborsCount = m_Grid3D->GetNeighborsFixedGrid(i,  neighborsBuffer, 1024);

        average += neighborsCount;

        int indexParticle = particleOrder[i].m_ParticleIndex;
        assert(m_PreviousDensity[indexParticle] != 0.0f);
        m_Pressure[indexParticle] = slmath::vec4(0.0f);
        m_Density[indexParticle] = 0.0f;



        for (int j = 0; j < neighborsCount; j++)
        {
            int indexParticleNeighbor = particleOrder[neighborsBuffer[j]].m_ParticleIndex;
            assert(m_PreviousDensity[indexParticleNeighbor] != 0.0f);
            slmath::vec4 separation = m_ParticlePositions[indexParticle] - m_ParticlePositions[indexParticleNeighbor];
            float sqrDistance = slmath::dot(separation, separation);
             
            slmath::vec4 normal = separation;

            // Add assert if none neighbors are false positives
            //assert(sqrDistance <= sqrMaxDistance);
            
            if (sqrDistance < m_SphParameters.m_H)
            {
                float distance = 0.0;
                if (sqrDistance > 0.0f)
                 {
                    distance = sqrtf(sqrDistance);
                    normal /= distance;
                 }

                m_Density[indexParticle] += m_SphParameters.m_Mass * ( 15 / (PI*m_SphParameters.m_H*m_SphParameters.m_H*m_SphParameters.m_H) ) * ((1 - distance / m_SphParameters.m_H)*(1 - distance / m_SphParameters.m_H)*(1 - distance / m_SphParameters.m_H));
                m_Pressure[indexParticle] += m_SphParameters.m_Mass / m_PreviousDensity[indexParticleNeighbor] * 
                ((m_SphParameters.m_GazConstant * m_PreviousDensity[indexParticle] + m_SphParameters.m_GazConstant * m_PreviousDensity[indexParticleNeighbor]) * 0.5f)
                 * (- 45 / (PI*m_SphParameters.m_H*m_SphParameters.m_H*m_SphParameters.m_H*m_SphParameters.m_H) ) * ((m_SphParameters.m_H - distance)*(m_SphParameters.m_H - distance)) * normal;

                 totalNeighborsCount++;
            }
        }

        assert(totalNeighborsCount != 0);
        m_Pressure[indexParticle] =  -(m_SphParameters.m_Mass / m_PreviousDensity[indexParticle]) * m_Pressure[indexParticle];


    }
     DEBUG_OUT("Average: \t" << average / m_ParticlesCount << "\n");
}

void SmoothedParticleHydrodynamics::SwapDensityBuffer()
{
   // Swap buffer
    float *buffer = m_Density;
    m_Density = m_PreviousDensity;
    m_PreviousDensity = buffer;
}
 

