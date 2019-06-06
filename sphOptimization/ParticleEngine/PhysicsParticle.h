

#pragma once

#ifndef PHYSICS_PARTICLE
#define PHYSICS_PARTICLE

#ifdef EXPORT_DLL
    #define MYPROJECT_API __declspec(dllexport)
#else
    #define MYPROJECT_API __declspec(dllimport)
#endif


struct ID3D11Device;
struct ID3D11Buffer;

// A vector 3
struct MYPROJECT_API vrVec3
{
    float x, y, z;
};

// A vector 4
struct MYPROJECT_API vrVec4 : public vrVec3
{
    float w;
};

// An Axis Aligned Bounding Volume
struct MYPROJECT_API vrAabb
{
    vrVec4 m_Min;
    vrVec4 m_Max;
};

// A sphere
struct MYPROJECT_API vrSphere
{
    vrVec3  m_Position;
    float   m_Radius;
};

// A distance constraint between two paricles
struct MYPROJECT_API vrSpring
{
    int     m_ParticleIndex1;
    int     m_ParticleIndex2;
    float   m_Distance;
    int     m_Pad;

    vrSpring():m_Pad(0) {}
};

// A particle accelerator
// Type 0 is a force field,
// Type 1 is a simple force, that uses only the directionvector
struct MYPROJECT_API vrAccelerator
{
    enum { FORCE_FIELD = 0, SIMPLE_FORCE = 1, ACCELRATOR_TYPES_COUNT};
    vrVec4          m_Position;
    vrVec4          m_Direction;
    float           m_Radius;
    unsigned int    m_Type;
};


class MYPROJECT_API PhysicsParticle
{

public:
    PhysicsParticle();
    ~PhysicsParticle();
    
    // Particles positions accesor
    vrVec4 *GetParticlePositions() const;
    vrVec4 *GetPreviousPositions() const;
    int GetParticlesCount() const;

    // Setters
    void SetParticlesMass(float mass);
    void SetParticlesGazConstant(float mass);
    void SetParticlesViscosity(float viscosity);
    void SetParticlesAcceleration(const vrVec4 &acceleration);
    void SetDamping(float damping);

    // Getters
    float GetParticlesMass() const;
    float GetParticlesGazConstant() const;
    float GetParticlesViscosity() const;

    // Initialize particles start position
    void Initialize(vrVec4 *positions,int positionsCount);

    // Intialize openCL device
    void InitializeOpenCL(  ID3D11Device *d3D11Device = 0 ,
                            ID3D11Buffer *d3D11buffer = 0);

    // Initialize data to the device
    void InitializeOpenClData();

    // Physics step
    void Simulate();

    // Release allocated memory
    void Release();

    // Dynamic grid used to test or to get neighbors information
    void CreateGrid(vrVec4 *positions, int positionsCount);
    void CreateGrid();
    int GetNeighbors(int currentIndex,  int *neighbors, int neighborsMaxCount);

    // Add collision geometry
    void AddInsideAabb(const vrAabb& aabb);
    void AddOutsideAabb(const vrAabb& aabb);
    void AddInsideSphere(const vrSphere& sphere);
    void AddOutsideSphere(const vrSphere& sphere);

    // Accesor to modify them, the returned ponters could change when 
    // collision geometries are added
    vrAabb    *GetInsideAabbs();
    vrSphere  *GetOutsideSpheres();
    int GetInsideAabbsCount() const;
    int GetOutsideSpheresCount() const;

    // Add a constraint between two particles
    void AddSpring(const vrSpring& spring);

    // Accelerators
    // Add an accelerator, to use to setup a force field or simple gravity
    void AddAccelerator(const vrAccelerator & accelerator);
    // Removes all accelerator
    void ClearAccelerators();
    void UpdateAccelerator(int index, const vrAccelerator& accelerator);

    
    // Physcal parameter
    // Grid and sph
    void SetEnableGridOnGPU(bool isCreatingGridOnGPU);
    void SetEnableSPHAndIntegrateOnGPU(bool sphAndIntegrateOnGPU);
    void SetEnableCollisionOnGPU(bool collisionOnGPU);
    void SetEnableSpringOnGPU(bool springOnGPU);
    void SetEnableAcceleratorOnGPU(bool acceleratorOnGPU);

    // Cloth only for springs
    void SetClothCount(int clothCount);

    // Enabling interoperability
    void SetIsUsingInteroperability(bool isUsingInteroperability);
    bool IsUsingInteroperability() const;

    // Running physics on the CPU, GPU is used by default
    void SetIsUsingCPU(bool isUsingCPU);

private:

    // Internal methods called in Simulate methods
    void CreateGridOnGPU();
    void SimuateSPHIntegrateOnGPU();
    void CollisionOnGPU();
    void SolveSpringOnGPU();
    void AcceleratorsOnGPU();

    // Exposed simulation parameter, cf accesors
    bool m_IsCreatingGridOnGPU;
    bool m_SPHAndIntegrateOnGPU;
    bool m_CollisionOnGPU;
    bool m_SpringOnGPU;
    bool m_AcceleratorOnGPU;

    // Internal boolean value without accesor
    bool m_IsUsingGrid3D;
    bool m_SPHSimulation;
    bool m_ContinuousIntegration;
    bool m_IsUsingAccelerator;
    bool m_IsIntegrating;
    bool m_IsSolvingSpring;
    bool m_IsColliding;

};


#endif // PHYSICS_PARTICLE