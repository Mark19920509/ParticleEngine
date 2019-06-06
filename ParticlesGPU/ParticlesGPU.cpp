#include "ParticlesGPU.hpp"

#include <slmath/slmath.h>
#include <assert.h>
#include <string>
#include <algorithm>

#include "File.hpp"
#include "ParticleEngine/SmoothedParticleHydrodynamics.h"
#include "ParticleEngine/ParticlesCollider.h"
#include "ParticleEngine/ParticlesSpring.h"
#include "ParticleEngine/ParticlesAccelerator.h"
#include "Utility/Utility.h"
#include "Utility/Timer.h"


#include <CL/cl_d3d11.h>
#include <CL/cl_d3d11_ext.h>
#include <CL/cl_ext.h>


#pragma warning( disable : 4996 )


clGetDeviceIDsFromD3D11KHR_fn       clGetDeviceIDsFromD3D11KHR      = NULL;
clCreateFromD3D11BufferKHR_fn		clCreateFromD3D11BufferKHR      = NULL;
clCreateFromD3D11Texture2DKHR_fn	clCreateFromD3D11Texture2DKHR   = NULL;
clCreateFromD3D11Texture3DKHR_fn    clCreateFromD3D11Texture3DKHR   = NULL;
clEnqueueAcquireD3D11ObjectsKHR_fn	clEnqueueAcquireD3D11ObjectsKHR = NULL;
clEnqueueReleaseD3D11ObjectsKHR_fn	clEnqueueReleaseD3D11ObjectsKHR = NULL;

clGetDeviceIDsFromD3D11NV_fn        clGetDeviceIDsFromD3D11NV      = NULL;
clCreateFromD3D11BufferNV_fn		clCreateFromD3D11BufferNV      = NULL;
clCreateFromD3D11Texture2DNV_fn		clCreateFromD3D11Texture2DNV   = NULL;
clCreateFromD3D11Texture3DNV_fn     clCreateFromD3D11Texture3DNV   = NULL;
clEnqueueAcquireD3D11ObjectsNV_fn	clEnqueueAcquireD3D11ObjectsNV = NULL;
clEnqueueReleaseD3D11ObjectsNV_fn	clEnqueueReleaseD3D11ObjectsNV = NULL;



#define INITPFN(x) \
    x = (x ## _fn)clGetExtensionFunctionAddress(#x);\
	if(!x) { printf("failed getting %s" #x); }


bool                ParticlesGPU::s_IsSetup             = false;
bool                ParticlesGPU::s_ProgramIsCompiled   = false;
bool                ParticlesGPU::s_IsAmdHardware       = false;

cl_context          ParticlesGPU::s_Context[s_ContextCount];
cl_device_id*       ParticlesGPU::s_Devices[s_ContextCount];
cl_program          ParticlesGPU::s_Program;
cl_platform_id      ParticlesGPU::s_PlatformId;


ParticlesGPU::ParticlesGPU() :
        m_ParticlesCount(0),
        m_LocalThreads(1),
        m_Positions(NULL),
        m_PreviousPositions(NULL),
        m_NeighborsInfo(NULL),
        m_Pressures(NULL),
        m_Density(NULL),
        m_DeviceId(0),
        m_ByteRWSupport(true),
        m_ClothCount(1),
        m_IsUsingInteroperability(false),
        m_ContextIndex(0),
        m_PositionsBuffer(NULL),
        m_PreviousPositionsBuffer(NULL),
        m_NeighborsInfoBuffer(NULL),
        m_NeighborsInfoBuffer2(NULL),
        m_OutputBuffer(NULL),
        m_SpheresBuffer(NULL),
        m_SpheresInBuffer(NULL),
        m_AabbsBuffer(NULL),
        m_SpringsBuffer(NULL),
        m_AcceleratorsBuffer(NULL),
        m_GridInfoBuffer(NULL),
        m_MinMaxBuffer(NULL),
        m_PreviousDensityBuffer(NULL),
        m_OutputDensityBuffer(NULL),
        m_SphParametersBuffer(NULL),
        m_StartsAnimationBuffer(NULL),
        m_EndsAnimationBuffer(NULL),
        m_CreateGridKernel(NULL),
        m_SortLocalKernel(NULL),
        m_MergeSortKernel(NULL),
        m_MinMaxKernel(NULL),
        m_SPHIntegrateKernel(NULL),
        m_CopyBufferKernel(NULL),
        m_CollisionKernel(NULL),
        m_SpringKernel(NULL),
        m_AcceleratorKernel(NULL),
        m_AnimationKernel(NULL)
{
}

ParticlesGPU::~ParticlesGPU()
{
}

void ParticlesGPU::SetClothCount(int clothCount)
{
    m_ClothCount = clothCount;
}

void ParticlesGPU::SetAnimationTime(float animationTime)
{
    m_AnimationTime = animationTime;
}

void ParticlesGPU::SetIsUsingCPU(bool isUsingCPU)
{
    if (isUsingCPU)
    {
        m_ContextIndex = 1;
    }
    else
    {
        m_ContextIndex = 0;
    }
}

bool ParticlesGPU::IsUsingCPU() const
{
    return m_ContextIndex == 1;
}

void ParticlesGPU::SetIsUsingInteroperability(bool isUsingInteroperability)
{
    m_IsUsingInteroperability = isUsingInteroperability;
}

bool ParticlesGPU::IsUsingInteroperability() const
{
    return m_IsUsingInteroperability;
}

void ParticlesGPU::UpdateInputCreateGrid(slmath::vec4 *positions, slmath::vec4 *outMin, int *neighborInfo, int *neighborInfo2, int *gridInfo)
{
    m_GridInfo          = reinterpret_cast<cl_int4*>(gridInfo);
    m_Positions         = reinterpret_cast<cl_float4*>(positions);
    m_Pressures         = reinterpret_cast<cl_float4*>(outMin);
    m_NeighborsInfo     = reinterpret_cast<cl_int2*>(neighborInfo);
    m_NeighborsInfo2    = reinterpret_cast<cl_int2*>(neighborInfo2);
}



void ParticlesGPU::CheckOutputMinMax()
{
#ifdef DEBUG
    for(int i = 0; i < m_ParticlesCount; i++)
    {
        assert(m_Pressures[0].s[0] <= m_Positions[i].s[0]);
        assert(m_Pressures[0].s[1] <= m_Positions[i].s[1]);
        assert(m_Pressures[0].s[2] <= m_Positions[i].s[2]);

        assert(m_Pressures[1].s[0] >= m_Positions[i].s[0]);
        assert(m_Pressures[1].s[1] >= m_Positions[i].s[1]);
        assert(m_Pressures[1].s[2] >= m_Positions[i].s[2]);
    }

#endif // DEBUG
}

void ParticlesGPU::CheckOutputGrid()
{
#ifdef DEBUG
    for(int i = 0; i < m_ParticlesCount - 1; i++)
    {
        int smaller = m_NeighborsInfo[i].s[1];
        int bigger = m_NeighborsInfo[i+1].s[1];
        bool isSorted = (smaller <= bigger);
        assert(isSorted);
    }

#endif // DEBUG
}

void ParticlesGPU::UpdateInput(slmath::vec4 *positions, float * density, int *neighborInfo, int *gridInfo, slmath::vec4 *outPressure)
{
    assert(gridInfo[0] == m_ParticlesCount);

#ifdef DEBUG
    for(int i = 0; i < m_ParticlesCount; i++)
    {
        assert(density[i] > 0.0f);
        assert(slmath::check(positions[i]));
    }
#endif // DEBUG

    m_GridInfo = reinterpret_cast<cl_int4*>(gridInfo);
    m_Density = reinterpret_cast<float*>(density);
    m_Positions = reinterpret_cast<cl_float4*>(positions);
    m_NeighborsInfo = reinterpret_cast<cl_int2*>(neighborInfo);
    m_Pressures = reinterpret_cast<cl_float4*>(outPressure);
}


void ParticlesGPU::UpdateInputSPH(  slmath::vec4 *positions, slmath::vec4 *previousPositions, float * density, 
                                    int *neighborInfo, int *gridInfo, const SphParameters& sphParameters)
{
    assert(gridInfo[0] == m_ParticlesCount);

    
    m_Positions         = reinterpret_cast<cl_float4*>(positions);
    m_PreviousPositions = reinterpret_cast<cl_float4*>(previousPositions);
    m_Density           = reinterpret_cast<float*>(density);
    m_GridInfo          = reinterpret_cast<cl_int4*>(gridInfo);
    m_NeighborsInfo     = reinterpret_cast<cl_int2*>(neighborInfo);
    m_SphParameters     = &sphParameters;
}

void ParticlesGPU::UpdateInputCollision(slmath::vec4 *positions, slmath::vec4 *previousPositions, int particlesCount, 
                                        const Sphere *spheres, int spheresCount,
                                        const Sphere *spheresIn, int spheresInCount,
                                        const Aabb *aabbs, int aabbsCount)
{
    assert( spheresCount <= MAX_SHAPES_COUNT);
    assert( spheresInCount <= MAX_SHAPES_COUNT);
    assert( aabbsCount <= MAX_SHAPES_COUNT);
    UNUSED_PARAMETER(particlesCount);
    assert(particlesCount == m_ParticlesCount);

    m_Positions         = reinterpret_cast<cl_float4*>(positions);
    m_PreviousPositions = reinterpret_cast<cl_float4*>(previousPositions);
    m_Spheres           = reinterpret_cast<const cl_float4*>(spheres);
    m_SpheresIn         = reinterpret_cast<const cl_float4*>(spheresIn);
    m_Aabbs             = reinterpret_cast<const cl_float4*>(aabbs);

    m_ShapesCount.s[0] = spheresCount;
    m_ShapesCount.s[1] = spheresInCount;
    m_ShapesCount.s[2] = 2 * aabbsCount;
    m_ShapesCount.s[3] = 0;
}

void ParticlesGPU::UpdateInputSpring(slmath::vec4 *positions, int particlesCount, const Spring *springs, int springsCount)
{
    UNUSED_PARAMETER(particlesCount);
    UNUSED_PARAMETER(springsCount);
    assert(particlesCount == m_ParticlesCount);
    assert(springsCount == m_SpringsCount);
    
    m_Positions = reinterpret_cast<cl_float4*>(positions);

    m_Springs = springs;
}

void ParticlesGPU::UpdateInputAccelerator(  slmath::vec4 *positions, slmath::vec4 *previousPositions, int particlesCount, 
                                            const Accelerator *accelerators, int acceleratorsCount)
{
    UNUSED_PARAMETER(particlesCount);
    assert(particlesCount == m_ParticlesCount);
    assert(acceleratorsCount == m_AcceleratorsCount);
    assert(acceleratorsCount > 0);

    m_AcceleratorsCount = acceleratorsCount;
    
    m_Positions = reinterpret_cast<cl_float4*>(positions);
    m_PreviousPositions = reinterpret_cast<cl_float4*>(previousPositions);
    m_Accelerators = accelerators;
}

void ParticlesGPU::UpdateInputAnimation(slmath::vec4 *positions, slmath::vec4 *ends, int particlesCount)
{
    UNUSED_PARAMETER(particlesCount);
    assert(particlesCount == m_ParticlesCount);
    m_Positions     = reinterpret_cast<cl_float4*>(positions);
    m_EndsAnimation = reinterpret_cast<cl_float4*>(ends);
}

int ParticlesGPU::BuildOpenCLProgram(int contextIndex)
{
    if (s_ProgramIsCompiled)
    {
        return CL_SUCCESS;
    }

    cl_int status = CL_SUCCESS;
    File kernelFile;
    
    std::string kernelFileName("openCL\\ParticlesGPU_Kernels.cl");
    if(!kernelFile.open(kernelFileName.c_str()))
    {
        DEBUG_OUT("Failed to load kernel file: " << kernelFileName.c_str() << std::endl);
        assert(false && "Failed to load kernel file");
    }
    const char * source = kernelFile.source().c_str();
    size_t sourceSize[] = {strlen(source)};
    s_Program = clCreateProgramWithSource(s_Context[contextIndex],
                                        1,
                                        &source,
                                        sourceSize,
                                        &status);
    assert(status == CL_SUCCESS);


    
    // Create a cl program executable for all the devices specified
    status = clBuildProgram(s_Program, 1, s_Devices[contextIndex], "", NULL, NULL);
    if(status != CL_SUCCESS)
    {
        if(status == CL_BUILD_PROGRAM_FAILURE)
        {
            cl_int logStatus;
            char buildLog[s_TextSizeOnStack];

            memset(buildLog, 0, s_TextSizeOnStack);

            logStatus = clGetProgramBuildInfo (
                            s_Program, 
                            s_Devices[contextIndex][0], 
                            CL_PROGRAM_BUILD_LOG, 
                            s_TextSizeOnStack, 
                            buildLog, 
                            NULL);

            if(logStatus != CL_SUCCESS)
            {
                free(buildLog);
                return logStatus;
            }

            DEBUG_OUT( " \n\t\t\tBUILD LOG\n");
            DEBUG_OUT( " ************************************************\n");
            DEBUG_OUT( buildLog << std::endl);
            DEBUG_OUT( " ************************************************\n");
            free(buildLog);
            assert(logStatus != CL_SUCCESS  && "clGetProgramBuildInfo failed.");
        }

        assert(status == CL_SUCCESS  && "clBuildProgram failed.");
    }
    // Success to be sure to compile just once
    s_ProgramIsCompiled = true;
    return status;
}



int ParticlesGPU::Setup(ID3D11Device* d3D11Device /*= NULL*/)
{

    if (s_IsSetup == true)
    {
        return 0;
    }

    s_IsSetup = true;
    cl_int status = 0;
    s_PlatformId = NULL;

    cl_uint numPlatforms = 5;

    if (0 < numPlatforms) 
    {
        cl_platform_id* platforms = new cl_platform_id[numPlatforms];
        status = clGetPlatformIDs(numPlatforms, platforms, &numPlatforms);
        assert(status == CL_SUCCESS  && "clGetPlatformIDs failed.");

        char platformName[100];
        for (unsigned i = 0; i < numPlatforms; ++i) 
        {
            status = clGetPlatformInfo(platforms[i],
                                        CL_PLATFORM_VENDOR,
                                        sizeof(platformName),
                                        platformName,
                                        NULL);
            assert(status == CL_SUCCESS  && "clGetPlatformInfo failed.");
            s_PlatformId = platforms[i];
        }
        delete[] platforms;
    }

    assert(int(s_PlatformId != NULL) && "NULL platform found\n");

    // Check platform info
    {    
        char platrformInfo[s_TextSizeOnStack];
        clGetPlatformInfo(s_PlatformId,  
                        CL_PLATFORM_EXTENSIONS,  
                        s_TextSizeOnStack,  
                        platrformInfo,  
                        NULL);

        if(strstr(platrformInfo, "cl_khr_d3d11_sharing") != NULL)
        {
            s_IsAmdHardware = true;
        }
        DEBUG_OUT("Platform Info :\n" << platrformInfo << "\n");
    }

    cl_context_properties cps[3] = 
    {
        CL_CONTEXT_PLATFORM, 
        (cl_context_properties)s_PlatformId, 
        0
    };

    cl_device_type dTypes[] = {CL_DEVICE_TYPE_GPU, CL_DEVICE_TYPE_CPU};

    for (int contextIndex = 0; contextIndex < s_ContextCount; contextIndex++)
    {

        s_Context[contextIndex] = clCreateContextFromType(
                      cps,
                      dTypes[contextIndex],
                      NULL,
                      NULL,
                      &status);
        assert(status == CL_SUCCESS  && "clCreateContextFromType failed.");

       
        if (d3D11Device != NULL && dTypes[contextIndex] == CL_DEVICE_TYPE_GPU)
        {

            
             // Initialize extension functions for D3D11
             // See the clGetExtensionFunctionAddress() documentation
             // for more details 
        
            if (s_IsAmdHardware)
            {
                INITPFN(clGetDeviceIDsFromD3D11KHR);
                // Get device from DirectX11
                cl_uint deviceCount = 0;

                status = clGetDeviceIDsFromD3D11KHR (  s_PlatformId,  
                                              CL_D3D11_DEVICE_KHR,  
                                              d3D11Device,  
                                              CL_PREFERRED_DEVICES_FOR_D3D11_KHR,  
                                              0,  
                                              NULL,  
                                              &deviceCount);
                assert(status == 0 && "clGetDeviceIDsFromD3D11KHR failed.");

                s_Devices[contextIndex] = new cl_device_id[deviceCount];
                assert(s_Devices[contextIndex] != NULL && "Failed to allocate memory (devices).");


                status = clGetDeviceIDsFromD3D11KHR (  s_PlatformId,  
                                              CL_D3D11_DEVICE_KHR,  
                                              d3D11Device,  
                                              CL_PREFERRED_DEVICES_FOR_D3D11_KHR,  
                                              deviceCount,  
                                              s_Devices[contextIndex],
                                              NULL);
                assert(status == 0 && "clGetDeviceIDsFromD3D11KHR failed.");
            }
            else
            {
                INITPFN(clGetDeviceIDsFromD3D11NV);
                     cl_uint deviceCount = 0;

                status = clGetDeviceIDsFromD3D11NV (  s_PlatformId,  
                                              CL_D3D11_DEVICE_NV,  
                                              d3D11Device,  
                                              CL_PREFERRED_DEVICES_FOR_D3D11_NV,  
                                              0,  
                                              NULL,  
                                              &deviceCount);
                assert(status == 0 && "clGetDeviceIDsFromD3D11NV failed.");

                s_Devices[contextIndex] = new cl_device_id[deviceCount];
                assert(s_Devices[contextIndex] != NULL && "Failed to allocate memory (devices).");


                status = clGetDeviceIDsFromD3D11NV (  s_PlatformId,  
                                              CL_D3D11_DEVICE_NV,  
                                              d3D11Device,  
                                              CL_PREFERRED_DEVICES_FOR_D3D11_NV,  
                                              deviceCount,  
                                              s_Devices[contextIndex],
                                              NULL);
                assert(status == 0 && "clGetDeviceIDsFromD3D11NV failed.");
            }

            

        }
        else
        {
            // First, get the size of device list data
            size_t deviceListSize = 0;
	        int status = 0;
	        status = clGetContextInfo(
                         s_Context[contextIndex],
                         CL_CONTEXT_DEVICES, 
                         0, 
                         NULL, 
                         &deviceListSize);

            assert(status == CL_SUCCESS  && "clGetContextInfo failed.");

            unsigned int deviceCount = (unsigned int)(deviceListSize / sizeof(cl_device_id));

            // 
	        // Now allocate memory for device list based on the size we got earlier
	        // Note that this memory is allocated to a pointer which is a argument
	        // so it must not be deleted inside this function. The Sample implementer
	        // has to delete the devices pointer in the host code at clean up
	        // 

            s_Devices[contextIndex] = new cl_device_id[deviceCount];

            assert(s_Devices[contextIndex] != NULL && "Failed to allocate memory (devices).");

            // Now, get the device list data
            status = clGetContextInfo(s_Context[contextIndex], 
                         CL_CONTEXT_DEVICES, 
                         deviceListSize,
                         s_Devices[contextIndex],
                         NULL);

            assert(status == CL_SUCCESS && "FclGetGetContextInfo failed");
        }

        // Check extensions
        {   
            char extensions[s_TextSizeOnStack];
            assert(extensions != NULL && "Failed to allocate memory(extensions)");

            status = clGetDeviceInfo(
                        s_Devices[contextIndex][0], 
                        CL_DEVICE_EXTENSIONS,
                        sizeof(char) * s_TextSizeOnStack,
                        extensions,
                        NULL);
            assert(status == CL_SUCCESS && "clGetDeviceIDs(CL_DEVICE_EXTENSIONS) failed");

            // Check particular extension
            if(!strstr(extensions, "cl_khr_byte_addressable_store"))
            {
                assert(false && "Device does not support cl_khr_byte_addressable_store extension!\n");
            }

            DEBUG_OUT( "Device extensions:\n" << extensions << "\n");
        }

        // create a CL program using the kernel source 
        status = BuildOpenCLProgram(contextIndex);
        assert(status == CL_SUCCESS && "BuildOpenCLProgram failed");
    }
    return status;
}

int ParticlesGPU::CreeateKernels()
{
    
    cl_int status = 0;

    if (m_Pipeline.m_IsCreatingGridOnGPU)
    {
        // get a kernel object handle for a kernel with the given name
        m_CreateGridKernel = clCreateKernel(s_Program, "CreateGrid", &status);
        assert(status == CL_SUCCESS && "clCreateKernel CreateGrid failed");

        // get a kernel object handle for a kernel with the given name
        m_SortLocalKernel = clCreateKernel(s_Program, "SortLocalGridCells", &status);
        assert(status == CL_SUCCESS && "clCreateKernel SortLocalGridCells failed");

        // get a kernel object handle for a kernel with the given name
        m_MergeSortKernel = clCreateKernel(s_Program, "MergeSortGridCells", &status);
        assert(status == CL_SUCCESS && "clCreateKernel m_MergeSortKernel failed");

        // get a kernel object handle for a kernel with the given name
        m_MinMaxKernel = clCreateKernel(s_Program, "ComputeMinMax", &status);
        assert(status == CL_SUCCESS && "clCreateKernel ComputeMinMax failed");
    }

    if (m_Pipeline.m_SPHAndIntegrateOnGPU)
    {
        // get a kernel object handle for a kernel with the given name
        m_SPHIntegrateKernel = clCreateKernel(s_Program, "ComputeSPH", &status);
        assert(status == CL_SUCCESS && "clCreateKernel ComputeSPH failed");
    }

    if (m_Pipeline.m_CollisionOnGPU)
    {
        // get a kernel object handle for a kernel with the given name
        m_CollisionKernel = clCreateKernel(s_Program, "ComputeCollision", &status);
        assert(status == CL_SUCCESS && "clCreateKernel ComputeCollision failed");
    }


    if (m_Pipeline.m_SpringOnGPU)
    {
        // get a kernel object handle for a kernel with the given name
        m_SpringKernel = clCreateKernel(s_Program, "ComputeSpring", &status);
        assert(status == CL_SUCCESS && "clCreateKernel ComputeSpring failed");
    }

    if (m_Pipeline.m_AcceleratorOnGPU)
    {
        // get a kernel object handle for a kernel with the given name
        m_AcceleratorKernel = clCreateKernel(s_Program, "ComputeAccelerator", &status);
        assert(status == CL_SUCCESS && "clCreateKernel ComputeAccelerator failed");
    }

    if (m_Pipeline.m_IsUsingAnimation)
    {
        // get a kernel object handle for a kernel with the given name
        m_AnimationKernel = clCreateKernel(s_Program, "ComputeAnimation", &status);
        assert(status == CL_SUCCESS && "clCreateKernel ComputeAnimation failed");
    }

    if (m_Pipeline.m_SPHAndIntegrateOnGPU || m_Pipeline.m_IsUsingAnimation)
    {
        // get a kernel object handle for a kernel with the given name
        m_CopyBufferKernel = clCreateKernel(s_Program, "CopyBuffer", &status);
        assert(status == CL_SUCCESS && "clCreateKernel CopyBuffer failed");
    }
    
    return status;
}

int ParticlesGPU::CreateBuffers(ID3D11Buffer *d3D11buffer)
{
    cl_int status = 0;

    // Create command queue
    cl_command_queue_properties prop = 0;
    m_CommandQueue = clCreateCommandQueue(
            s_Context[m_ContextIndex], 
            s_Devices[m_ContextIndex][m_DeviceId], 
            prop,
            &status);
    assert(status == CL_SUCCESS && "clCreateCommandQueue failed");


    m_ShapesCount.s[0] = MAX_SHAPES_COUNT;
    m_ShapesCount.s[1] = MAX_SHAPES_COUNT;    
    m_ShapesCount.s[2] = 2 * MAX_SHAPES_COUNT;
    

    // Input buffer
    if (d3D11buffer == NULL || m_IsUsingInteroperability == false)
    {
        m_PositionsBuffer = clCreateBuffer( s_Context[m_ContextIndex],
                                            CL_MEM_READ_WRITE,
                                            sizeof(cl_float4) * m_ParticlesCount,
                                            NULL,
                                            &status);
        assert(status == CL_SUCCESS && "clCreateBuffer failed. (m_PositionsBuffer)");
    }
    else
    {
        if (s_IsAmdHardware)
        {
            INITPFN(clCreateFromD3D11BufferKHR);
            INITPFN(clCreateFromD3D11Texture2DKHR);
            INITPFN(clCreateFromD3D11Texture3DKHR);
            INITPFN(clEnqueueAcquireD3D11ObjectsKHR);
            INITPFN(clEnqueueReleaseD3D11ObjectsKHR);

            m_PositionsBuffer = clCreateFromD3D11BufferKHR (   s_Context[m_ContextIndex],  
                                                                CL_MEM_READ_WRITE,
                                                                d3D11buffer,  
                                                                &status);

            assert(status == CL_SUCCESS &&  "clCreateFromD3D11BufferKHR failed. (m_PositionsBuffer)");
        }
        else
        {
            INITPFN(clCreateFromD3D11BufferNV);
            INITPFN(clCreateFromD3D11Texture2DNV);
            INITPFN(clCreateFromD3D11Texture3DNV);
            INITPFN(clEnqueueAcquireD3D11ObjectsNV);
            INITPFN(clEnqueueReleaseD3D11ObjectsNV);

            m_PositionsBuffer = clCreateFromD3D11BufferNV (   s_Context[m_ContextIndex],  
                                                                CL_MEM_READ_WRITE,
                                                                d3D11buffer,  
                                                                &status);

            assert(status == CL_SUCCESS &&  "clCreateFromD3D11BufferNV failed. (m_PositionsBuffer)");
        }
    }

     // Input buffer
    m_PreviousPositionsBuffer = clCreateBuffer(
        s_Context[m_ContextIndex],
        CL_MEM_READ_WRITE,
        sizeof(cl_float4) * m_ParticlesCount,
        NULL,
        &status);
    assert(status == CL_SUCCESS &&  "clCreateBuffer failed. (m_PreviousPositionsBuffer)");


    if (m_Pipeline.m_IsUsingAnimation)
    {
        // Input buffer starts animation
        m_StartsAnimationBuffer = clCreateBuffer(
            s_Context[m_ContextIndex],
            CL_MEM_READ_ONLY,
            sizeof(cl_float4) * m_ParticlesCount,
            NULL,
            &status);
        assert(status == CL_SUCCESS &&  "clCreateBuffer failed. (m_StartsAnimationBuffer)");

        // Input buffer ends animation
        m_EndsAnimationBuffer = clCreateBuffer(
            s_Context[m_ContextIndex],
            CL_MEM_READ_ONLY,
            sizeof(cl_float4) * m_ParticlesCount,
            NULL,
            &status);
        assert(status == CL_SUCCESS &&  "clCreateBuffer failed. (m_EndsAnimationBuffer)");
    }

    if (m_Pipeline.m_IsCreatingGridOnGPU && m_Pipeline.m_SPHAndIntegrateOnGPU)
    {
        // Density double buffer
        m_PreviousDensityBuffer  = clCreateBuffer(
            s_Context[m_ContextIndex],
            CL_MEM_READ_WRITE,
            sizeof(cl_float) * m_ParticlesCount,
            NULL,
            &status);
        assert(status == CL_SUCCESS &&  "clCreateBuffer failed. (m_PreviousDensityBuffer)");

        // Density double buffer
        m_OutputDensityBuffer  = clCreateBuffer(
            s_Context[m_ContextIndex],
            CL_MEM_READ_WRITE,
            sizeof(cl_float) * m_ParticlesCount,
            NULL,
            &status);
        assert(status == CL_SUCCESS &&  "clCreateBuffer failed. (m_OutputDensityBuffer)");

        // Input buffer for SPH OutputBuffer fo CreateGrid
        m_NeighborsInfoBuffer = clCreateBuffer(
            s_Context[m_ContextIndex],
            CL_MEM_READ_WRITE,
            sizeof(cl_int2) * m_ParticlesCount,
            NULL,
            &status);
        assert(status == CL_SUCCESS &&  "clCreateBuffer failed. (m_NeighborsInfoBuffer)");

        // Internal GPU buffer
        m_NeighborsInfoBuffer2 = clCreateBuffer(
            s_Context[m_ContextIndex],
            CL_MEM_READ_WRITE,
            sizeof(cl_int2) * m_ParticlesCount,
            NULL,
            &status);
        assert(status == CL_SUCCESS &&  "clCreateBuffer failed. (m_NeighborsInfoBuffer2)");
        
        m_SphParametersBuffer = clCreateBuffer( s_Context[m_ContextIndex],
                                            CL_MEM_READ_ONLY,
                                            sizeof(SphParameters),
                                            NULL,
                                            &status);
        assert(status == CL_SUCCESS &&  "clCreateBuffer failed. (m_SphParametersBuffer)");

        m_GridInfoBuffer = clCreateBuffer(
        s_Context[m_ContextIndex],
        CL_MEM_READ_WRITE,
        sizeof(cl_int4) ,
        NULL,
        &status);
        assert(status == CL_SUCCESS &&  "clCreateBuffer failed. (m_GridInfoBuffer)");

        m_MinMaxBuffer  = clCreateBuffer(
            s_Context[m_ContextIndex],
            CL_MEM_READ_WRITE,
            2 * sizeof(cl_float4),
            NULL,
            &status);
        assert(status == CL_SUCCESS &&  "clCreateBuffer failed. (m_MinMaxBuffer)");

        // Output positions buffer used to synch global barrier
        m_OutputBuffer = clCreateBuffer(s_Context[m_ContextIndex], 
                                        CL_MEM_READ_WRITE,
                                        sizeof(cl_float4) * m_ParticlesCount,
                                        NULL, 
                                        &status);
        assert(status == CL_SUCCESS &&  "clCreateBuffer failed. (m_OutputBuffer)");
    }

    if (m_Pipeline.m_CollisionOnGPU)
    {
        // Shapes for collision
        m_SpheresBuffer = clCreateBuffer(
            s_Context[m_ContextIndex],
            CL_MEM_READ_ONLY,
            sizeof(cl_float4) * m_ShapesCount.s[0],
            NULL,
            &status);
        assert(status == CL_SUCCESS &&  "clCreateBuffer failed. (m_SpheresBuffer)");

        m_SpheresInBuffer = clCreateBuffer(
            s_Context[m_ContextIndex],
            CL_MEM_READ_ONLY,
            sizeof(cl_float4) * m_ShapesCount.s[1],
            NULL,
            &status);
        assert(status == CL_SUCCESS &&  "clCreateBuffer failed. (m_SpheresInBuffer)");

        m_AabbsBuffer = clCreateBuffer(
            s_Context[m_ContextIndex],
            CL_MEM_READ_ONLY,
            sizeof(cl_float4) * m_ShapesCount.s[2],
            NULL,
            &status);
        assert(status == CL_SUCCESS &&  "clCreateBuffer failed. (m_AabbsBuffer)");
    }


    // Spring
    if (m_Pipeline.m_SpringOnGPU && m_SpringsCount > 0)
    {
        m_SpringsBuffer = clCreateBuffer(   s_Context[m_ContextIndex],
                                            CL_MEM_READ_ONLY,
                                            sizeof(Spring) * m_SpringsCount,
                                            NULL,
                                            &status);
        assert(status == CL_SUCCESS &&  "clCreateBuffer failed. (m_SpringsBuffer)");
    }
    else
    {
        m_SpringsBuffer = NULL;
    }

    if (m_AcceleratorsCount > 0)
    {
    
        m_AcceleratorsBuffer = clCreateBuffer( s_Context[m_ContextIndex],
                                                CL_MEM_READ_ONLY,
                                                sizeof(Accelerator) * m_AcceleratorsCount,
                                                NULL,
                                                &status);
        assert(status == CL_SUCCESS &&  "clCreateBuffer failed. (m_AcceleratorsBuffer)");
    }
    else 
    {
        m_AcceleratorsBuffer = NULL;
    }
    
    return status;
}


int ParticlesGPU::InitializeCreateGrid()
{
    assert(m_Pipeline.m_IsCreatingGridOnGPU);

    cl_int status;
    cl_event writeEvt;

     // Enqueue write from m_Positions to m_PositionsBuffer
    status = clEnqueueWriteBuffer(m_CommandQueue,
                                  m_PositionsBuffer,
                                  CL_FALSE,
                                  0,
                                  sizeof(cl_float4) * m_ParticlesCount,
                                  m_Positions,
                                  0,
                                  NULL,
                                  &writeEvt);

    assert(status == CL_SUCCESS &&  "clEnqueueWriteBuffer failed. (m_PositionsBuffer)");

     status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // No clFinish command queue because there is one before running kernel !

    return status;

}

int ParticlesGPU::CreateGrid()
{

    Timer::GetInstance()->StartTimerProfile();
    // This kernel won't support les than 10 work items
    assert(m_LocalThreads > 10);
    cl_int status;


    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFinish failed.");

    // Find the minimum and the maximum position

    
    // Setup kernel arguments
    status = clSetKernelArg(m_MinMaxKernel, 
                            0, 
                            sizeof(cl_mem), 
                            (void *)&m_PositionsBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_PositionsBuffer)");

     // local memory
    status = clSetKernelArg(m_MinMaxKernel,
                            1,
                            2 * m_LocalThreads * 4 * sizeof(float),
                            NULL);
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (localPos)");

    // global memory
    status = clSetKernelArg(m_MinMaxKernel, 
                            2,
                            sizeof(cl_mem), 
                            (void *)&m_MinMaxBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_MinMaxBuffer)");


    size_t globalThreadMinMax = m_GlobalThreads / 2;
    size_t localThreadMinMax = std::min(globalThreadMinMax, m_LocalThreads);
    
    cl_event ndrEvt;

    // Enqueue m_MinMaxKernel kernel
    status = clEnqueueNDRangeKernel(
        m_CommandQueue,
        m_MinMaxKernel,
        1,
        NULL,
        &globalThreadMinMax,
        &localThreadMinMax,
        0,
        NULL,
        &ndrEvt);
    assert(status == CL_SUCCESS &&  "clEnqueueNDRangeKernel failed.");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFinish failed.");

    // Create grid

    // Setup kernel arguments
    status = clSetKernelArg(m_CreateGridKernel, 
                            0, 
                            sizeof(cl_mem), 
                            (void *)&m_PositionsBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_PositionsBuffer)");

     
    // global memory
    status = clSetKernelArg(m_CreateGridKernel, 
                            1,
                            sizeof(cl_mem), 
                            (void *)&m_MinMaxBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_MinMaxBuffer)");

    // Neighbors info
    status = clSetKernelArg(m_CreateGridKernel,
                            2,
                            sizeof(cl_mem), 
                            (void *)&m_NeighborsInfoBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_NeighborsInfoBuffer)");

    status = clSetKernelArg(m_CreateGridKernel,
                            3,
                            sizeof(cl_mem), 
                            (void *)&m_GridInfoBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_GridInfoBuffer)");

    // Enqueue the kernel to create the grid
    status = clEnqueueNDRangeKernel(
        m_CommandQueue,
        m_CreateGridKernel,
        1,
        NULL,
        &m_GlobalThreads,
        &m_LocalThreads,
        0,
        NULL,
        &ndrEvt);
    assert(status == CL_SUCCESS &&  "clEnqueueNDRangeKernel failed.");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFinish failed.");

    Timer::GetInstance()->StopTimerProfile("Create Grid : Grid Creation");

    return status;
}

void ParticlesGPU::TestSort()
{
assert(m_Pipeline.m_IsCreatingGridOnGPU);
// Test
#ifdef DEBUG
    for (int i = 0; i < m_ParticlesCount; i++)
    {
        m_NeighborsInfo[i].s[0] = m_ParticlesCount - 1 - i;
        m_NeighborsInfo[i].s[1] = rand() % 16;
    }


    // Enqueue write from m_Positions to m_PositionsBuffer
    cl_event writeEvt;
    cl_int status;
    status = clEnqueueWriteBuffer(m_CommandQueue,
                                  m_NeighborsInfoBuffer,
                                  CL_FALSE,
                                  0, 
                                  sizeof(cl_int2) * m_ParticlesCount,
                                  m_NeighborsInfo, 
                                  0, 
                                  NULL, 
                                  &writeEvt);

    assert(status == CL_SUCCESS);

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS);

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    assert(status == CL_SUCCESS);

    // End Test
#endif DEBUG
}

int ParticlesGPU::SortGrid()
{
    cl_int status;
    cl_event ndrEvt;
    
    Timer::GetInstance()->StartTimerProfile();

    // Sort grid cells

    // Neighbors info
    status = clSetKernelArg(m_SortLocalKernel,
                            0,
                            sizeof(cl_mem), 
                            (void *)&m_NeighborsInfoBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_NeighborsInfoBuffer)");

    // Neighbors info buffer
    status = clSetKernelArg(m_SortLocalKernel,
                            1,
                            sizeof(cl_mem), 
                            (void *)&m_NeighborsInfoBuffer2); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_NeighborsInfoBuffer 2 )");


    // Enqueue the kernel to sort the grid
    status = clEnqueueNDRangeKernel(
        m_CommandQueue,
        m_SortLocalKernel,
        1,
        NULL,
        &m_GlobalThreads,
        &m_LocalThreads,
        0,
        NULL,
        &ndrEvt);
    assert(status == CL_SUCCESS &&  "clEnqueueNDRangeKernel failed.");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFinish failed.");


    // Selection Merge sort on GPU

    // Neighbors info
    status = clSetKernelArg(m_MergeSortKernel,
                            0,
                            sizeof(cl_mem), 
                            (void *)&m_NeighborsInfoBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_NeighborsInfoBuffer)");

    // Neighbors info buffer
    status = clSetKernelArg(m_MergeSortKernel,
                            1,
                            sizeof(cl_mem), 
                            (void *)&m_NeighborsInfoBuffer2); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_NeighborsInfoBuffer 2 )");


    // Counters and / or for selection
    status = clSetKernelArg(m_MergeSortKernel,
                            2,
                            m_LocalThreads * sizeof(cl_int2), 
                            NULL); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (counters)");


    // Enqueue the kernel to merge the sort
    status = clEnqueueNDRangeKernel(
        m_CommandQueue,
        m_MergeSortKernel,
        1,
        NULL,
        &m_GlobalThreads,
        &m_LocalThreads,
        0,
        NULL,
        &ndrEvt);
    assert(status == CL_SUCCESS &&  "clEnqueueNDRangeKernel failed.");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFinish failed.");

    // Enqueue the results to application pointer    
    cl_event readEvt3;
    status = clEnqueueReadBuffer(
        m_CommandQueue, 
        m_GridInfoBuffer, 
        CL_FALSE,
        0,
        sizeof(cl_int4),
        m_GridInfo,
        0,
        NULL,
        &readEvt3);
    assert(status == CL_SUCCESS &&  "clEnqueueReadBuffer failed.");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFinish failed.");

    // This info is requiered for SPH
    m_GridInfo[0].s[0] = m_ParticlesCount;

    
    Timer::GetInstance()->StopTimerProfile("Create Grid : Sort on GPU");
    // Test function
    #ifdef DEBUG
        // Enqueue the results to application pointer
        cl_event readEvt2;
        status = clEnqueueReadBuffer(
            m_CommandQueue, 
            m_NeighborsInfoBuffer2,
            CL_FALSE,
            0,
            m_ParticlesCount * sizeof(cl_int2),
            m_NeighborsInfo,
            0,
            NULL,
            &readEvt2);
        assert(status == CL_SUCCESS &&  "clEnqueueReadBuffer failed.");

        status = clFlush(m_CommandQueue);
        assert(status == CL_SUCCESS &&  "clFlush failed.");

        // All the command in queue must be finihed before continuing to process
        status = clFinish(m_CommandQueue);
        assert(status == CL_SUCCESS &&  "clFinish failed.");

        CheckOutputGrid();
    #endif // DEBUG

    return status;
}


int ParticlesGPU::runKernelCreateGrid()
{
    assert(m_Pipeline.m_IsCreatingGridOnGPU);
    int status = CreateGrid();
    assert(status == CL_SUCCESS &&  "Grid Creation.");
    status = SortGrid();
    assert(status == CL_SUCCESS &&  "Sort Grid.");
    return status;
}


int ParticlesGPU::runKernelMinTest()
{
    assert(m_Pipeline.m_IsCreatingGridOnGPU);
#ifdef DEBUG
    cl_int status;
    cl_event ndrEvt;

    // Enqueue write from m_Positions to m_PositionsBuffer
    cl_event writeEvt;
    status = clEnqueueWriteBuffer(m_CommandQueue, 
                                  m_PositionsBuffer, 
                                  CL_TRUE,
                                  0, 
                                  sizeof(cl_float4) * m_ParticlesCount,
                                  m_Positions, 
                                  0, 
                                  NULL, 
                                  &writeEvt);
    assert(status == CL_SUCCESS &&  "clEnqueueWriteBuffer failed. (m_PositionsBuffer)");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");


    // Setup kernel arguments
    status = clSetKernelArg(m_MinMaxKernel, 
                            0, 
                            sizeof(cl_mem), 
                            (void *)&m_PositionsBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_PositionsBuffer)");

     // local memory
    status = clSetKernelArg(m_MinMaxKernel,
                            1,
                            2 * m_LocalThreads * 4 * sizeof(float),
                            NULL);
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (localPos)");

    // global memory
    status = clSetKernelArg(m_MinMaxKernel, 
                            2,
                            sizeof(cl_mem), 
                            (void *)&m_MinMaxBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_MinMaxBuffer)");


    // 
    //Enqueue a kernel run call.
    //
    status = clEnqueueNDRangeKernel(
        m_CommandQueue,
        m_MinMaxKernel,
        1,
        NULL,
        &m_GlobalThreads,
        &m_LocalThreads,
        0,
        NULL,
        &ndrEvt);
    assert(status == CL_SUCCESS &&  "clEnqueueNDRangeKernel failed.");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFinish failed.");
    
    // Enqueue the results to application pointer
    cl_event readEvt;
    status = clEnqueueReadBuffer(
        m_CommandQueue, 
        m_MinMaxBuffer, 
        CL_FALSE,
        0,
        2 * sizeof(cl_float4),
        m_Pressures,
        0,
        NULL,
        &readEvt);
    assert(status == CL_SUCCESS &&  "clEnqueueReadBuffer failed.");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFinish failed.");

    
    CheckOutputMinMax();
    
#endif // DEBUG
    return CL_SUCCESS;
}


int ParticlesGPU::InitializeKernelSPH()
{
    assert(m_Pipeline.m_SPHAndIntegrateOnGPU);
    m_SwapBufferSPH = false;
    cl_int status;
    // Enqueue write from m_Positions to m_PositionsBuffer
    cl_event writeEvt;
    status = clEnqueueWriteBuffer(m_CommandQueue, 
                                  m_PositionsBuffer, 
                                  CL_FALSE,
                                  0, 
                                  sizeof(cl_float4) * m_ParticlesCount,
                                  m_Positions, 
                                  0, 
                                  NULL, 
                                  &writeEvt);
    assert(status == CL_SUCCESS &&  "clEnqueueWriteBuffer failed. (m_PositionsBuffer)");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // Enqueue write from m_PreviousPositions to m_PreviousPositionsBuffer
    cl_event writeEvt2;
    status = clEnqueueWriteBuffer(m_CommandQueue, 
                                  m_PreviousPositionsBuffer, 
                                  CL_FALSE,
                                  0, 
                                  sizeof(cl_float4) * m_ParticlesCount,
                                  m_PreviousPositions, 
                                  0, 
                                  NULL, 
                                  &writeEvt2);
    assert(status == CL_SUCCESS &&  "clEnqueueWriteBuffer failed. (m_PreviousPositionsBuffer)");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");


    // Enqueue write from m_Density to m_PreviousDensityBuffer
    cl_event writeEvt3;
    status = clEnqueueWriteBuffer(m_CommandQueue, 
                                  m_PreviousDensityBuffer, 
                                  CL_FALSE,
                                  0, 
                                  sizeof(cl_float) * m_ParticlesCount,
                                  m_Density, 
                                  0, 
                                  NULL, 
                                  &writeEvt3);
    assert(status == CL_SUCCESS &&  "clEnqueueWriteBuffer failed. (m_PreviousDensityBuffer)");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // No clFinish command queue because there is one before running kernel !

    return status;
}

int ParticlesGPU::runKernelSPH()
{
    assert(m_Pipeline.m_SPHAndIntegrateOnGPU);
    cl_int status;
    cl_event writeEvt4;
    status = clEnqueueWriteBuffer(m_CommandQueue, 
                                  m_GridInfoBuffer, 
                                  CL_TRUE,
                                  0, 
                                  sizeof(cl_int4),
                                  m_GridInfo, 
                                  0, 
                                  NULL, 
                                  &writeEvt4);
    assert(status == CL_SUCCESS &&  "clEnqueueWriteBuffer failed. (m_GridInfoBuffer)");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // Enqueue write from m_SphParameters to m_SphParametersBuffer
    cl_event writeEvt5;
    status = clEnqueueWriteBuffer(m_CommandQueue, 
                                  m_SphParametersBuffer, 
                                  CL_TRUE,
                                  0, 
                                  sizeof(SphParameters),
                                  m_SphParameters, 
                                  0, 
                                  NULL, 
                                  &writeEvt5);
    assert(status == CL_SUCCESS &&  "clEnqueueWriteBuffer failed. (m_GridInfoBuffer)");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFinish failed.");

    // Setup kernel arguments
    status = clSetKernelArg(m_SPHIntegrateKernel, 
                            0, 
                            sizeof(cl_mem), 
                            (void *)&m_PositionsBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_PositionsBuffer)");

    status = clSetKernelArg(m_SPHIntegrateKernel, 
                            1, 
                            sizeof(cl_mem), 
                            (void *)&m_PreviousPositionsBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_PreviousPositionsBuffer)");

    status = clSetKernelArg(m_SPHIntegrateKernel, 
                            2, 
                            sizeof(cl_mem), 
                            (void *)&m_NeighborsInfoBuffer2); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_NeighborsInfoBuffer2)");

    status = clSetKernelArg(m_SPHIntegrateKernel, 
                            3, 
                            sizeof(cl_mem), 
                            (void *)&m_GridInfoBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_GridInfo)");

    status = clSetKernelArg(m_SPHIntegrateKernel, 
                            4, 
                            sizeof(cl_mem), 
                            (void *)&m_SphParametersBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_SphParameters)");

    status = clSetKernelArg(m_SPHIntegrateKernel, 
                            5, 
                            sizeof(cl_mem), 
                            (void *)&m_OutputBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_OutputBuffer)");

    cl_int firstIndex = m_SwapBufferSPH ? 7 : 6;
    cl_int secondIndex = m_SwapBufferSPH ? 6 : 7;

    status = clSetKernelArg(m_SPHIntegrateKernel, 
                            firstIndex, 
                            sizeof(cl_mem), 
                            (void *)&m_PreviousDensityBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_PreviousDensityBuffer)");

    status = clSetKernelArg(m_SPHIntegrateKernel, 
                            secondIndex, 
                            sizeof(cl_mem), 
                            (void *)&m_OutputDensityBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_OutputDensityBuffer)");

    // Swap buffer next time
    m_SwapBufferSPH = ! m_SwapBufferSPH;

    // 
    //Enqueue a kernel run call.
    //
    cl_event ndrEvt;
    status = clEnqueueNDRangeKernel(
        m_CommandQueue,
        m_SPHIntegrateKernel,
        1,
        NULL,
        &m_GlobalThreads,
        &m_LocalThreads,
        0,
        NULL,
        &ndrEvt);
    assert(status == CL_SUCCESS &&  "clEnqueueNDRangeKernel failed.");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFinish failed.");

    // Copy position output in the positions

    // Setup kernel arguments
    status = clSetKernelArg(m_CopyBufferKernel, 
                            0, 
                            sizeof(cl_mem), 
                            (void *)&m_OutputBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_PositionsBuffer)");

    status = clSetKernelArg(m_CopyBufferKernel, 
                            1, 
                            sizeof(cl_mem), 
                            (void *)&m_PositionsBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_OutputBuffer)");


    //
    //Enqueue a kernel run call.
    //
    cl_event ndrEvt2;
    status = clEnqueueNDRangeKernel(
        m_CommandQueue,
        m_CopyBufferKernel,
        1,
        NULL,
        &m_GlobalThreads,
        &m_LocalThreads,
        0,
        NULL,
        &ndrEvt2);
    assert(status == CL_SUCCESS &&  "clEnqueueNDRangeKernel failed.");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFinish failed.");

    if ( ! m_IsUsingInteroperability)
    {
        // Enqueue the results to application pointer
        cl_event readEvt;
        status = clEnqueueReadBuffer(
            m_CommandQueue, 
            m_PositionsBuffer, 
            CL_FALSE,
            0,
            m_ParticlesCount* sizeof(cl_float4),
            m_Positions,
            0,
            NULL,
            &readEvt);
        assert(status == CL_SUCCESS &&  "clEnqueueReadBuffer failed.");

        status = clFlush(m_CommandQueue);
        assert(status == CL_SUCCESS &&  "clFlush failed.");


        // Enqueue the results to application pointer
        cl_event readEvt2;
        status = clEnqueueReadBuffer(
            m_CommandQueue, 
            m_PreviousPositionsBuffer, 
            CL_FALSE,
            0,
            m_ParticlesCount* sizeof(cl_float4),
            m_PreviousPositions,
            0,
            NULL,
            &readEvt2);
        assert(status == CL_SUCCESS &&  "clEnqueueReadBuffer failed.");

        status = clFlush(m_CommandQueue);
        assert(status == CL_SUCCESS &&  "clFlush failed.");

        // All the command in queue must be finihed before continuing to process
        status = clFinish(m_CommandQueue);
        assert(status == CL_SUCCESS &&  "clFinish failed.");
    }

// 

    return status;
}

int ParticlesGPU::InitializeKernelCollision()
{
    cl_int status;

    // Enqueue write from m_Positions to m_PositionsBuffer
    cl_event writeEvt;
    status = clEnqueueWriteBuffer(m_CommandQueue, 
                                  m_PositionsBuffer, 
                                  CL_FALSE,
                                  0, 
                                  sizeof(cl_float4) * m_ParticlesCount,
                                  m_Positions, 
                                  0, 
                                  NULL, 
                                  &writeEvt);
    assert(status == CL_SUCCESS &&  "clEnqueueWriteBuffer failed. (m_PositionsBuffer)");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // Enqueue write from m_Positions to m_PreviousPositionsBuffer
    cl_event writeEvt4;
    status = clEnqueueWriteBuffer(m_CommandQueue, 
                                  m_PreviousPositionsBuffer, 
                                  CL_FALSE,
                                  0, 
                                  sizeof(cl_float4) * m_ParticlesCount,
                                  m_PreviousPositions, 
                                  0, 
                                  NULL, 
                                  &writeEvt4);
    assert(status == CL_SUCCESS &&  "clEnqueueWriteBuffer failed. (m_PreviousPositionsBuffer)");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // No clFinish command queue because there is one before running kernel !

    return status;

}


int ParticlesGPU::runKernelCollision()
{
    assert(m_Pipeline.m_CollisionOnGPU);
    cl_int status;

    // Enqueue write from m_Spheres to m_SpheresBuffer
    cl_event writeEvt2;
    status = clEnqueueWriteBuffer(m_CommandQueue, 
                                  m_SpheresBuffer, 
                                  CL_FALSE,
                                  0, 
                                  sizeof(cl_float4) * slmath::max(m_ShapesCount.s[0], 1),
                                  m_Spheres, 
                                  0, 
                                  NULL, 
                                  &writeEvt2);
    assert(status == CL_SUCCESS &&  "clEnqueueWriteBuffer failed. (m_Spheres)");

     status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // Enqueue write from m_Spheres to m_SpheresInBuffer
    cl_event writeEvt3;
    status = clEnqueueWriteBuffer(m_CommandQueue, 
                                  m_SpheresInBuffer, 
                                  CL_FALSE,
                                  0, 
                                  sizeof(cl_float4) * slmath::max(m_ShapesCount.s[1], 1),
                                  m_SpheresIn, 
                                  0, 
                                  NULL, 
                                  &writeEvt3);
    assert(status == CL_SUCCESS &&  "clEnqueueWriteBuffer failed. (m_Spheres)");

     status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");


    // Enqueue write from m_Aabbs to m_AabbsBuffer
    cl_event writeEvt4;
    status = clEnqueueWriteBuffer(m_CommandQueue, 
                                  m_AabbsBuffer,
                                  CL_FALSE,
                                  0, 
                                  sizeof(cl_float4) * slmath::max(m_ShapesCount.s[2], 1),
                                  m_Aabbs, 
                                  0, 
                                  NULL, 
                                  &writeEvt4);
    assert(status == CL_SUCCESS &&  "clEnqueueWriteBuffer failed. (m_AabbsBuffer)");

     status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFinish failed.");

    // Setup kernel arguments
    status = clSetKernelArg(m_CollisionKernel, 
                            0, 
                            sizeof(cl_mem), 
                            (void *)&m_PositionsBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_PositionsBuffer)");

    status = clSetKernelArg(m_CollisionKernel, 
                            1, 
                            sizeof(cl_mem), 
                            (void *)&m_PreviousPositionsBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_PreviousPositionsBuffer)");

    status = clSetKernelArg(m_CollisionKernel, 
                            2,
                            sizeof(cl_mem), 
                            (void *)&m_SpheresBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_SpheresBuffer)");

    status = clSetKernelArg(m_CollisionKernel, 
                            3,
                            sizeof(cl_mem), 
                            (void *)&m_SpheresInBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_SpheresInBuffer)");

    status = clSetKernelArg(m_CollisionKernel, 
                            4, 
                            sizeof(cl_mem), 
                            (void *)&m_AabbsBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_AabbsBuffer)");

    status = clSetKernelArg(m_CollisionKernel, 
                            5, 
                            sizeof(cl_int4), 
                            (void *)&m_ShapesCount);
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_ShapesCount)");

    // 
    //Enqueue a kernel run call.
    //
    cl_event ndrEvt;
    status = clEnqueueNDRangeKernel(
        m_CommandQueue,
        m_CollisionKernel,
        1,
        NULL,
        &m_GlobalThreads,
        &m_LocalThreads,
        0,
        NULL,
        &ndrEvt);
    assert(status == CL_SUCCESS &&  "clEnqueueNDRangeKernel failed.");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFinish failed.");

    if ( ! m_IsUsingInteroperability)
    {
        // Enqueue the results to application pointer
        cl_event readEvt;
        status = clEnqueueReadBuffer(
            m_CommandQueue, 
            m_PositionsBuffer, 
            CL_FALSE,
            0,
            m_ParticlesCount* sizeof(cl_float4),
            m_Positions,
            0,
            NULL,
            &readEvt);
        assert(status == CL_SUCCESS &&  "clEnqueueReadBuffer failed.");

        status = clFlush(m_CommandQueue);
        assert(status == CL_SUCCESS &&  "clFlush failed.");


        // Enqueue the results to application pointer
        cl_event readEvt2;
        status = clEnqueueReadBuffer(
            m_CommandQueue, 
            m_PreviousPositionsBuffer, 
            CL_FALSE,
            0,
            m_ParticlesCount* sizeof(cl_float4),
            m_PreviousPositions,
            0,
            NULL,
            &readEvt2);
        assert(status == CL_SUCCESS &&  "clEnqueueReadBuffer failed.");

        status = clFlush(m_CommandQueue);
        assert(status == CL_SUCCESS &&  "clFlush failed.");

        // All the command in queue must be finihed before continuing to process
        status = clFinish(m_CommandQueue);
        assert(status == CL_SUCCESS &&  "clFinish failed.");
    }

    return status;
}


int ParticlesGPU::InitializeKernelSpring()
{
    assert(m_Pipeline.m_SpringOnGPU);
    cl_int status;
    // Enqueue write from m_Positions to m_PositionsBuffer
    cl_event writeEvt;
    status = clEnqueueWriteBuffer(m_CommandQueue, 
                                  m_PositionsBuffer, 
                                  CL_FALSE,
                                  0, 
                                  sizeof(cl_float4) * m_ParticlesCount,
                                  m_Positions, 
                                  0, 
                                  NULL, 
                                  &writeEvt);
    assert(status == CL_SUCCESS &&  "clEnqueueWriteBuffer failed. (m_PositionsBuffer)");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // Enqueue write from m_Springs to m_SpringsBuffer
    cl_event writeEvt3;
    status = clEnqueueWriteBuffer(m_CommandQueue, 
                                  m_SpringsBuffer, 
                                  CL_FALSE,
                                  0, 
                                  sizeof(Spring) * m_SpringsCount,
                                  m_Springs, 
                                  0, 
                                  NULL, 
                                  &writeEvt3);
    assert(status == CL_SUCCESS &&  "clEnqueueWriteBuffer failed. (m_Springs)");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // No clFinish command queue because there is one before running kernel !

    return status;
}

int ParticlesGPU::runKernelSpring()
{
    assert(m_Pipeline.m_SpringOnGPU);
    cl_int status;

    // All buffer must be written before sending args
    status = clFinish(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFinish failed.");
    
    // Setup kernel arguments
    status = clSetKernelArg(m_SpringKernel, 
                            0, 
                            sizeof(cl_mem), 
                            (void *)&m_PositionsBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_PositionsBuffer)");

    status = clSetKernelArg(m_SpringKernel, 
                            1, 
                            sizeof(cl_mem), 
                            (void *)&m_SpringsBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_SpringsBuffer)");

    status = clSetKernelArg(m_SpringKernel, 
                            2, 
                            sizeof(cl_float4) * m_ParticlesCount / m_ClothCount, // particles by cloth or work item 
                            NULL);
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (Local memory)");
    
    // 
    //Enqueue a kernel run call.
    //
    cl_event ndrEvt;
    status = clEnqueueNDRangeKernel(
        m_CommandQueue,
        m_SpringKernel,
        1,
        NULL,
        &m_GlobalThreadsSpring,
        &m_LocalThreadsSpring,
        0,
        NULL,
        &ndrEvt);
    assert(status == CL_SUCCESS &&  "clEnqueueNDRangeKernel failed.");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // Kernel must be finished before reading data
    status = clFinish(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFinish failed.");
    

    if (!m_IsUsingInteroperability)
    {
        // Enqueue the results to application pointer
        cl_event readEvt;
        status = clEnqueueReadBuffer(
            m_CommandQueue, 
            m_PositionsBuffer, 
            CL_FALSE,
            0,
            m_ParticlesCount* sizeof(cl_float4),
            m_Positions,
            0,
            NULL,
            &readEvt);
        assert(status == CL_SUCCESS &&  "clEnqueueReadBuffer failed.");

        status = clFlush(m_CommandQueue);
        assert(status == CL_SUCCESS &&  "clFlush failed.");

         // Block until all output data are ready
        status = clFinish(m_CommandQueue);
        assert(status == CL_SUCCESS &&  "clFinish failed.");
    }

    return status;
}

int ParticlesGPU::InitializeKernelAccelerator()
{
    cl_int status;

    // Enqueue write from m_Positions to m_PositionsBuffer
    cl_event writeEvt;
    status = clEnqueueWriteBuffer(m_CommandQueue, 
                                  m_PositionsBuffer, 
                                  CL_FALSE,
                                  0, 
                                  sizeof(cl_float4) * m_ParticlesCount,
                                  m_Positions, 
                                  0, 
                                  NULL, 
                                  &writeEvt);
    assert(status == CL_SUCCESS &&  "clEnqueueWriteBuffer failed. (m_PositionsBuffer)");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");


    // Enqueue write from m_PreviousPositions to m_PreviousPositionsBuffer
    cl_event writeEvt2;
    status = clEnqueueWriteBuffer(m_CommandQueue, 
                                  m_PreviousPositionsBuffer, 
                                  CL_FALSE,
                                  0, 
                                  sizeof(cl_float4) * m_ParticlesCount,
                                  m_PreviousPositions, 
                                  0, 
                                  NULL, 
                                  &writeEvt2);
    assert(status == CL_SUCCESS &&  "clEnqueueWriteBuffer failed. (m_PreviousPositionsBuffer)");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // No clFinish command queue because there is one before running kernel !

    return status;
}

int ParticlesGPU::runKernelAccelerator()
{
    assert(m_Pipeline.m_AcceleratorOnGPU);
    cl_int status;

    // Enqueue write from m_Accelerators to m_AcceleratorsBuffer
    cl_event writeEvt3;
    status = clEnqueueWriteBuffer(m_CommandQueue, 
                                  m_AcceleratorsBuffer, 
                                  CL_FALSE,
                                  0, 
                                  sizeof(Accelerator) * m_AcceleratorsCount,
                                  m_Accelerators, 
                                  0, 
                                  NULL, 
                                  &writeEvt3);
    assert(status == CL_SUCCESS &&  "clEnqueueWriteBuffer failed. (m_AcceleratorsBuffer)");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");


    // All buffer must be written before sending args
    status = clFinish(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFinish failed.");
    
    // Setup kernel arguments

    
    status = clSetKernelArg(m_AcceleratorKernel, 
                            0, 
                            sizeof(cl_mem), 
                            (void *)&m_PositionsBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_PositionsBuffer)");

    status = clSetKernelArg(m_AcceleratorKernel, 
                            1, 
                            sizeof(cl_mem), 
                            (void *)&m_PreviousPositionsBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_PreviousPositionsBuffer)");

    status = clSetKernelArg(m_AcceleratorKernel, 
                            2, 
                            sizeof(cl_mem), 
                            (void *)&m_AcceleratorsBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_AcceleratorsBuffer)");

    status = clSetKernelArg(m_AcceleratorKernel, 
                            3, 
                            sizeof(cl_int), 
                            (void *)&m_AcceleratorsCount); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_AcceleratorsCount)");

 
    // 
    //Enqueue a kernel run call.
    //
    cl_event ndrEvt;
    status = clEnqueueNDRangeKernel(
        m_CommandQueue,
        m_AcceleratorKernel,
        1,
        NULL,
        &m_GlobalThreads,
        &m_LocalThreads,
        0,
        NULL,
        &ndrEvt);
    assert(status == CL_SUCCESS &&  "clEnqueueNDRangeKernel failed.");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // Kernel must be finished before reading data
    status = clFinish(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFinish failed.");

    if (! m_IsUsingInteroperability)
    {
         // Enqueue the results to application pointer
        cl_event readEvt;
        status = clEnqueueReadBuffer(
            m_CommandQueue, 
            m_PreviousPositionsBuffer, 
            CL_FALSE,
            0,
            m_ParticlesCount* sizeof(cl_float4),
            m_PreviousPositions,
            0,
            NULL,
            &readEvt);
        assert(status == CL_SUCCESS &&  "clEnqueueReadBuffer failed.");

        status = clFlush(m_CommandQueue);
        assert(status == CL_SUCCESS &&  "clFlush failed.");


        // Enqueue the results to application pointer
        status = clEnqueueReadBuffer(
            m_CommandQueue, 
            m_PositionsBuffer, 
            CL_FALSE,
            0,
            m_ParticlesCount* sizeof(cl_float4),
            m_Positions,
            0,
            NULL,
            &readEvt);
        assert(status == CL_SUCCESS &&  "clEnqueueReadBuffer failed.");

        status = clFlush(m_CommandQueue);
        assert(status == CL_SUCCESS &&  "clFlush failed.");

         // Block until all output data are ready
        status = clFinish(m_CommandQueue);
        assert(status == CL_SUCCESS &&  "clFinish failed.");
    }
   

    return status;
}

int ParticlesGPU::InitializeKernelAnimation()
{
    assert(m_Pipeline.m_IsUsingAnimation);
    cl_int status;

    // Enqueue write from m_EndsAnimation to m_EndsAnimationBuffer
    cl_event writeEvt3;
    status = clEnqueueWriteBuffer(m_CommandQueue, 
                                  m_EndsAnimationBuffer, 
                                  CL_FALSE,
                                  0, 
                                  sizeof(cl_float4) * m_ParticlesCount,
                                  m_EndsAnimation, 
                                  0, 
                                  NULL, 
                                  &writeEvt3);
    assert(status == CL_SUCCESS &&  "clEnqueueWriteBuffer failed. (m_EndsAnimationBuffer)");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // Block until all output data are ready
    status = clFinish(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFinish failed.");

    return status;
}

int ParticlesGPU::runKernelAnimation()
{
    assert(m_Pipeline.m_IsUsingAnimation);
    cl_int status;

    if (m_AnimationTime == 0.0f)
    {
        // Setup kernel arguments
        status = clSetKernelArg(m_CopyBufferKernel, 
                                0, 
                                sizeof(cl_mem), 
                                (void *)&m_PositionsBuffer); 
        assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_PositionsBuffer)");

        status = clSetKernelArg(m_CopyBufferKernel, 
                                1, 
                                sizeof(cl_mem), 
                                (void *)&m_StartsAnimationBuffer); 
        assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_OutputBuffer)");


        //
        //Enqueue a kernel run call.
        //
        cl_event ndrEvt2;
        status = clEnqueueNDRangeKernel(
            m_CommandQueue,
            m_CopyBufferKernel,
            1,
            NULL,
            &m_GlobalThreads,
            &m_LocalThreads,
            0,
            NULL,
            &ndrEvt2);
        assert(status == CL_SUCCESS &&  "clEnqueueNDRangeKernel failed.");

        status = clFlush(m_CommandQueue);
        assert(status == CL_SUCCESS &&  "clFlush failed.");

    }



    // Setup kernel arguments
    status = clSetKernelArg(m_AnimationKernel, 
                            0, 
                            sizeof(cl_mem), 
                            (void *)&m_PositionsBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_PositionsBuffer)");

    status = clSetKernelArg(m_AnimationKernel, 
                            1, 
                            sizeof(cl_mem),
                            (void *)&m_StartsAnimationBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_StartsAnimationBuffer)");

    status = clSetKernelArg(m_AnimationKernel,
                            2, 
                            sizeof(cl_mem), 
                            (void *)&m_EndsAnimationBuffer); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_EndsAnimationBuffer)");

    status = clSetKernelArg(m_AnimationKernel,
                            3, 
                            sizeof(cl_float), 
                            (void *)&m_AnimationTime); 
    assert(status == CL_SUCCESS &&  "clSetKernelArg failed. (m_AnimationTime)");
   DEBUG_OUT("Animation Time GPU :\t" << m_AnimationTime << "\n");
    
    // 
    //Enqueue a kernel run call.
    //
    cl_event ndrEvt;
    status = clEnqueueNDRangeKernel(
        m_CommandQueue,
        m_AnimationKernel,
        1,
        NULL,
        &m_GlobalThreads,
        &m_LocalThreads,
        0,
        NULL,
        &ndrEvt);
    assert(status == CL_SUCCESS &&  "clEnqueueNDRangeKernel failed.");

    status = clFlush(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFlush failed.");

    // Kernel must be finished before reading data
    status = clFinish(m_CommandQueue);
    assert(status == CL_SUCCESS &&  "clFinish failed.");

    if (! m_IsUsingInteroperability)
    {
         // Enqueue the results to application pointer
        cl_event readEvt;
        status = clEnqueueReadBuffer(
            m_CommandQueue, 
            m_PositionsBuffer, 
            CL_FALSE,
            0,
            m_ParticlesCount* sizeof(cl_float4),
            m_Positions,
            0,
            NULL,
            &readEvt);
        assert(status == CL_SUCCESS &&  "clEnqueueReadBuffer failed.");

        status = clFlush(m_CommandQueue);
        assert(status == CL_SUCCESS &&  "clFlush failed.");

         // Block until all output data are ready
        status = clFinish(m_CommandQueue);
        assert(status == CL_SUCCESS &&  "clFinish failed.");
    }
    return status;
}


int ParticlesGPU::Initialize(    int particlesCount, int springsCount, int acceleratorsCount,
                                const PipelineDescription& pipeline, ID3D11Buffer *d3D11buffer /*= NULL*/)
{
    m_ParticlesCount    = particlesCount;
    m_SpringsCount      = springsCount;
    m_AcceleratorsCount = acceleratorsCount;
    m_Pipeline          = pipeline;

    int status = CreeateKernels();

    if(status != CL_SUCCESS)
    {
        assert(status == CL_SUCCESS && "ParticlesGPU::CreeateKernels failed");
        return status;
    }

    status = CreateBuffers(d3D11buffer);    
    
    if(status != CL_SUCCESS)
    {
        return status;
    }

    m_GlobalThreads = slmath::roundToPowerOf2(particlesCount);

    
    // Maximum work group size
    size_t maxWorkGroupSize;
    status = clGetDeviceInfo(
                    s_Devices[m_ContextIndex][m_DeviceId], 
                    CL_DEVICE_MAX_WORK_GROUP_SIZE,
                    sizeof(size_t),
                    &maxWorkGroupSize,
                    NULL);
   assert(status == CL_SUCCESS &&  "clGetDeviceIDs(CL_DEVICE_MAX_WORK_GROUP_SIZE) failed");

    m_LocalThreads  = slmath::min(slmath::roundToPowerOf2((particlesCount >> 1) + 1), maxWorkGroupSize);

    //Get max work item dimensions
    size_t maxWorkItemDims;
    status = clGetDeviceInfo(
                    s_Devices[m_ContextIndex][m_DeviceId], 
                    CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,
                    sizeof(cl_uint),
                    &maxWorkItemDims,
                    NULL);
    assert(status == CL_SUCCESS &&  "clGetDeviceIDs(CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS) failed");

    //Get max work item sizes
    size_t* maxWorkItemSizes = new size_t[maxWorkItemDims];
    status = clGetDeviceInfo(
                s_Devices[m_ContextIndex][m_DeviceId], 
                CL_DEVICE_MAX_WORK_ITEM_SIZES,
                sizeof(size_t) * maxWorkItemDims,
                maxWorkItemSizes,
                NULL);
    assert(status == CL_SUCCESS &&  "clGetDeviceIDs(CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS) failed");


    assert (m_LocalThreads <= maxWorkItemSizes[0] && m_LocalThreads <= maxWorkGroupSize && 
            "Unsupported: Device does not support requested number of work items.\n");

    m_WorkGroupCount = m_GlobalThreads / m_LocalThreads;
    m_Counters = new int[m_WorkGroupCount];

    int workItemsCount = m_SpringsCount / 12;

    m_GlobalThreadsSpring = slmath::roundToPowerOf2(workItemsCount );
    m_LocalThreadsSpring  = slmath::min(m_GlobalThreadsSpring / m_ClothCount, maxWorkGroupSize);

    assert (m_LocalThreadsSpring <= maxWorkItemSizes[0] && m_LocalThreadsSpring <= maxWorkGroupSize && 
            "Unsupported: Device does not support requested number of work items.\n");

    return status;
}

int ParticlesGPU::cleanup()
{

    delete []m_Counters;

    assert(m_ByteRWSupport);


    cl_int status = CL_SUCCESS;

    // Release openCL buffer
    if (m_PositionsBuffer)
    {
        status = clReleaseMemObject(m_PositionsBuffer);
        assert(status == CL_SUCCESS &&  "clReleaseMemObject failed.(m_PositionsBuffer)");
    }

    if (m_PreviousPositionsBuffer)
    {
        status = clReleaseMemObject(m_PreviousPositionsBuffer);
        assert(status == CL_SUCCESS &&  "clReleaseMemObject failed.(m_PreviousPositionsBuffer)");
    }

    if (m_PreviousDensityBuffer)
    {
        status = clReleaseMemObject(m_PreviousDensityBuffer);
        assert(status == CL_SUCCESS &&  "clReleaseMemObject failed.(m_PreviousDensityBuffer)");
    }

    if (m_OutputDensityBuffer)
    {
        status = clReleaseMemObject(m_OutputDensityBuffer);
        assert(status == CL_SUCCESS  &&  "clReleaseMemObject failed.(m_OutputDensityBuffer)");
    }

    if (m_OutputBuffer)
    {
        status = clReleaseMemObject(m_OutputBuffer);
        assert(status == CL_SUCCESS &&  "clReleaseMemObject failed.(m_OutputBuffer)");
    }

    if (m_SpringsCount > 0)
    {
        status = clReleaseMemObject(m_SpringsBuffer);
        assert(status == CL_SUCCESS &&  "clReleaseMemObject failed.(m_SpringsBuffer)");
    }
    
    if (m_AcceleratorsCount > 0)
    {
        status = clReleaseMemObject(m_AcceleratorsBuffer);
        assert(status == CL_SUCCESS &&  "clReleaseMemObject failed.(m_AcceleratorsBuffer)");
    }

    // Releases OpenCL kernels
    
    if (m_SPHIntegrateKernel)
    {
        status = clReleaseKernel(m_SPHIntegrateKernel);
        assert(status == CL_SUCCESS &&  "clReleaseKernel failed.(m_SPHIntegrateKernel)");
    }

    if (m_CollisionKernel)
    {
        status = clReleaseKernel(m_CollisionKernel);
        assert(status == CL_SUCCESS &&  "clReleaseKernel failed.(m_CollisionKernel)");
    }

    // Release command queue
    if (m_CommandQueue)
    {
        status = clReleaseCommandQueue(m_CommandQueue);
        assert(status == CL_SUCCESS &&  "clReleaseCommandQueue failed.(m_CommandQueue)");
    }

    return status;
}

int ParticlesGPU::StaticRelease()
{
    cl_int status = clReleaseProgram(s_Program);
    assert(status == CL_SUCCESS &&  "clReleaseProgram failed.(s_Program)");
    
    for (int contextIndex = 0; contextIndex < s_ContextCount; contextIndex)
    {
        status = clReleaseContext(s_Context[contextIndex]);
        assert(status == CL_SUCCESS &&  "clReleaseContext failed.");
        delete s_Devices[contextIndex];
    }
    s_IsSetup = false;

    return status;
}

