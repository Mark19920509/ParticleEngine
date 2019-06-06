
#ifndef PARTICLES_GPU
#define PARTICLES_GPU

/**
 * Heaer Files
 */




#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <SDKCommon.hpp>
#include <SDKApplication.hpp>
#include <SDKCommandArgs.hpp>
#include <SDKFile.hpp>

namespace slmath
{
    class vec4;
}

struct Aabb;
struct Sphere;
struct Spring;
struct Accelerator;
struct SphParameters;

struct ID3D11Device;
struct ID3D11Buffer;

#define MAX_SHAPES_COUNT 25

/**
* ParticlesGPU 
* Class implements 256 ParticlesGPU bin implementation 
* Derived from SDKSample base class
*/

class ParticlesGPU : public SDKSample
{
    cl_int  m_ParticlesCount;        /**< Size of ParticlesGPU */
    cl_int  m_SpringsCount;         /**< Size of ParticlesGPU */
    cl_int  m_AcceleratorsCount;

    cl_int  m_ClothCount;

    size_t  m_LocalThreads;        /**< Number of threads in a Group */
    size_t  m_GlobalThreads;
    size_t  m_LocalThreadsSpring;
    size_t  m_GlobalThreadsSpring;
    size_t  m_WorkGroupCount;

    size_t  m_RealGridSize;

    bool m_IsUsingInteroperability;
    bool m_IsUsingCPU;

    // Swap buffer
    bool m_SwapBufferSPH;
    
    int         *m_Counters;
    cl_int4     *m_GridInfo;
    cl_int4     m_ShapesCount;

    //Host buffers
    cl_float4           *m_Positions;
    cl_float4           *m_PreviousPositions;
    cl_int2             *m_NeighborsInfo;
    cl_int2             *m_NeighborsInfo2;
    cl_float4           *m_Pressures;
    cl_float            *m_Density;
    const cl_float4     *m_Spheres;
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
    cl_mem  m_AabbsBuffer;
    cl_mem  m_SpringsBuffer;
    cl_mem  m_AcceleratorsBuffer;
    cl_mem  m_GridInfoBuffer;
    cl_mem  m_MinMaxBuffer;
    cl_mem  m_PreviousDensityBuffer;
    cl_mem  m_OutputDensityBuffer;

    cl_mem  m_SphParametersBuffer;
    cl_mem  m_RealGridBuffer;


    cl_double kernelTime;           /**< Time for host implementation */
    cl_double setupTime;            /**< Time for host implementation */

    cl_bool byteRWSupport;          /**< Flag for byte-addressable store */



    //CL objects
    cl_context m_Context;             /**< CL m_Context */
    cl_device_id *m_Devices;          /**< CL device list */
    cl_command_queue m_CommandQueue;  /**< CL command queue */
    cl_program m_Program;             /**< CL m_Program  */
    cl_platform_id m_PlatformId;      /**< CL platform */
    

    // Kernels
    cl_kernel m_CreateGridKernel;
    cl_kernel m_SortLocalKernel;
    cl_kernel m_MergeSortKernel;
    cl_kernel m_MinMaxKernel;
    cl_kernel m_ClearRealGridKernel;
    cl_kernel m_RealGridKernel;
    cl_kernel m_SPHIntegrateKernel;
    cl_kernel m_CollisionKernel;
    cl_kernel m_SpringKernel;
    cl_kernel m_AcceleratorKernel;

    
    // Kernel info
    streamsdk::KernelWorkGroupInfo  m_CreateGridKernelInfo,
                                    m_SortLocalKernelInfo,
                                    m_MergeSortKernelInfo,
                                    m_MinKernelInfo,
                                    m_ClearRealGridKernelInfo,
                                    m_RealGridKernelInfo,
                                    m_SPHIntegrateKernelInfo, 
                                    m_CollisionKernelInfo, 
                                    m_SpringKernelInfo,
                                    m_AcceleratorKernelInfo;
    // Device info
    streamsdk::SDKDeviceInfo m_DeviceInfo;

    

public:
    
    ParticlesGPU(const char* name)
        : SDKSample(name),
        m_ParticlesCount(0),
        m_LocalThreads(1),
        kernelTime(0),
        setupTime(0),
        m_Positions(NULL),
        m_PreviousPositions(NULL),
        m_NeighborsInfo(NULL),
        m_Pressures(NULL),
        m_Density(NULL),
        m_Devices(NULL),
        byteRWSupport(true),
        m_ClothCount(1),

        m_IsUsingInteroperability(false),
        m_IsUsingCPU(false),
        m_RealGridSize(128 * 128 * 128)
    {
    }

    ~ParticlesGPU(){}

    void SetIsUsingCPU(bool isUsingCPU);
    bool IsUsingCPU() const;
    void SetIsUsingInteroperability(bool isUsingInteroperability);
    bool IsUsingInteroperability() const;

    // Function used only for cloth simulation
    void SetClothCount(int clothCount);

    int setup(  int particlesCount, int springsCount, int acceleratorsCount,
                ID3D11Device *d3D11Device = NULL, ID3D11Buffer *d3D11buffer = NULL);

    // Initialize
    int InitializeCreateGrid();
    int InitializeKernelSPH();
    int InitializeKernelCollision();
    int InitializeKernelAccelerator();
    int InitializeKernelSpring();

    // Different available kernels
    int runKernelCreateGrid();
    int runKernelMinTest();
    int runKernelRealGrid();
    int runKernelClearRealGrid();
    int runKernelSPH();
    int runKernelCollision();
    int runKernelSpring();
    int runKernelAccelerator();

    // Update functions 
    void UpdateInputCreateGrid(slmath::vec4 *positions, slmath::vec4 *outMin, int *neighborInfo, int *neighborInfo2, int *gridInfo);
    void UpdateInput(slmath::vec4 *positions, float * density, int *neighborInfo, int *gridInfo, slmath::vec4 *outPressure);
    void UpdateInputSPH(slmath::vec4 *positions, slmath::vec4 *previousPositions, float * density, int *neighborInfo, int *gridInfo, const SphParameters& sphParameters);
    void UpdateInputCollision(slmath::vec4 *positions, slmath::vec4 *previousPositions, int particlesCount, 
                                            const Sphere *spheres, int spheresCount, 
                                            const Aabb *aabbs, int aabbsCount);
    void UpdateInputSpring(slmath::vec4 *positions, int particlesCount, const Spring *springs, int springsCount);
    void UpdateInputAccelerator(slmath::vec4 *positions, slmath::vec4 *previousPositions, int particlesCount, 
                                const Accelerator *accelerators, int acceleratorsCount);

    int cleanup();


private:
    int  setupCL(ID3D11Device *d3D11Device);
    int  CreateBuffers(ID3D11Buffer *d3D11buffer);
    int  CreeateKernels();
    void CheckOutputMinMax();
    void CheckOutputGrid();
    void UpdateOutputSPH();

    int SortGrid();
    int CreateGrid();

    void TestSort();
};



#endif  //PARTICLES_GPU
