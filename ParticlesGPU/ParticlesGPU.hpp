
#ifndef PARTICLES_GPU
#define PARTICLES_GPU

#include <CL/cl.h>
#include "ParticleEngine/PipelineDescription.h"

namespace slmath
{
    class vec4;
}

struct Aabb;
struct Sphere;
struct Spring;
struct Accelerator;
struct SphParameters;
struct PipelineDescription;

struct ID3D11Device;
struct ID3D11Buffer;

#define MAX_SHAPES_COUNT 25


class ParticlesGPU
{
    cl_int  m_ParticlesCount;
    cl_int  m_SpringsCount;
    cl_int  m_AcceleratorsCount;

    cl_int  m_ClothCount;

    size_t  m_LocalThreads;
    size_t  m_GlobalThreads;
    size_t  m_LocalThreadsSpring;
    size_t  m_GlobalThreadsSpring;
    size_t  m_WorkGroupCount;

    bool m_IsUsingInteroperability;
    int  m_ContextIndex;

    // Swap buffer
    bool m_SwapBufferSPH;
    
    int         *m_Counters;
    cl_int4     *m_GridInfo;
    cl_int4     m_ShapesCount;
    cl_float    m_AnimationTime;

    //Host buffers
    cl_float4           *m_Positions;
    cl_float4           *m_PreviousPositions;
    cl_float4           *m_EndsAnimation;
    cl_int2             *m_NeighborsInfo;
    cl_int2             *m_NeighborsInfo2;
    cl_float4           *m_Pressures;
    cl_float            *m_Density;
    const cl_float4     *m_Spheres;
    const cl_float4     *m_SpheresIn;
    const cl_float4     *m_Aabbs;
    const Spring        *m_Springs;
    const Accelerator   *m_Accelerators;
    const SphParameters *m_SphParameters;


    //Device buffers
    cl_mem  m_PositionsBuffer;
    cl_mem  m_PreviousPositionsBuffer;
    cl_mem  m_NeighborsInfoBuffer;
    cl_mem  m_NeighborsInfoBuffer2;
    cl_mem  m_OutputBuffer;
    cl_mem  m_SpheresBuffer;
    cl_mem  m_SpheresInBuffer;
    cl_mem  m_AabbsBuffer;
    cl_mem  m_SpringsBuffer;
    cl_mem  m_AcceleratorsBuffer;
    cl_mem  m_GridInfoBuffer;
    cl_mem  m_MinMaxBuffer;
    cl_mem  m_PreviousDensityBuffer;
    cl_mem  m_OutputDensityBuffer;
    cl_mem  m_SphParametersBuffer;
    cl_mem  m_StartsAnimationBuffer;
    cl_mem  m_EndsAnimationBuffer;


    cl_bool m_ByteRWSupport;


    // CPU and GPU context
    const static int s_ContextCount = 1;
    // CL objects
    static cl_context          s_Context[s_ContextCount];
    static cl_device_id*       s_Devices[s_ContextCount];
    static cl_program          s_Program;
    static cl_platform_id      s_PlatformId;
    static bool                s_IsSetup;
    static bool                s_ProgramIsCompiled;
    static bool                s_IsAmdHardware;

    cl_uint             m_DeviceId;
    cl_command_queue    m_CommandQueue;
    
    
    // Kernels
    cl_kernel m_CreateGridKernel;
    cl_kernel m_SortLocalKernel;
    cl_kernel m_MergeSortKernel;
    cl_kernel m_MinMaxKernel;
    cl_kernel m_SPHIntegrateKernel;
    cl_kernel m_CopyBufferKernel;
    cl_kernel m_CollisionKernel;
    cl_kernel m_SpringKernel;
    cl_kernel m_AcceleratorKernel;
    cl_kernel m_AnimationKernel;

    PipelineDescription m_Pipeline;
    
public:
    
    ParticlesGPU();
    ~ParticlesGPU();

    void SetIsUsingCPU(bool isUsingCPU);
    bool IsUsingCPU() const;
    void SetIsUsingInteroperability(bool isUsingInteroperability);
    bool IsUsingInteroperability() const;

    // Function used only for cloth simulation
    void SetClothCount(int clothCount);

    // Function used only for animation
    void SetAnimationTime(float animationTime);

    static int Setup(ID3D11Device *d3D11Device = NULL);
    static int StaticRelease();

    int Initialize(int particlesCount, int springsCount, int acceleratorsCount,
                   const PipelineDescription& pipeline, ID3D11Buffer *d3D11buffer = NULL);

    // Initialize
    int InitializeCreateGrid();
    int InitializeKernelSPH();
    int InitializeKernelCollision();
    int InitializeKernelAccelerator();
    int InitializeKernelSpring();
    int InitializeKernelAnimation();

    // Different available kernels
    int runKernelCreateGrid();
    int runKernelMinTest();
    int runKernelSPH();
    int runKernelCollision();
    int runKernelSpring();
    int runKernelAccelerator();
    int runKernelAnimation();

    // Update functions 
    void UpdateInputCreateGrid(slmath::vec4 *positions, slmath::vec4 *outMin, int *neighborInfo, int *neighborInfo2, int *gridInfo);
    void UpdateInput(slmath::vec4 *positions, float * density, int *neighborInfo, int *gridInfo, slmath::vec4 *outPressure);
    void UpdateInputSPH(slmath::vec4 *positions, slmath::vec4 *previousPositions, float * density, int *neighborInfo, int *gridInfo, const SphParameters& sphParameters);
    void UpdateInputCollision(slmath::vec4 *positions, slmath::vec4 *previousPositions, int particlesCount, 
                                            const Sphere *spheres, int spheresCount,
                                            const Sphere *spheresIn, int spheresInCount,
                                            const Aabb *aabbs, int aabbsCount);
    void UpdateInputSpring(slmath::vec4 *positions, int particlesCount, const Spring *springs, int springsCount);
    void UpdateInputAccelerator(slmath::vec4 *positions, slmath::vec4 *previousPositions, int particlesCount, 
                                const Accelerator *accelerators, int acceleratorsCount);
    void UpdateInputAnimation(slmath::vec4 *positions, slmath::vec4 *ends, int particlesCount);

    int cleanup();


private:
    static int BuildOpenCLProgram(int contextIndex);
    int  CreateBuffers(ID3D11Buffer *d3D11buffer);
    int  CreeateKernels();
    void CheckOutputMinMax();
    void CheckOutputGrid();

    int SortGrid();
    int CreateGrid();
    void TestSort();

    const static int s_TextSizeOnStack = 10240;
};



#endif  //PARTICLES_GPU
