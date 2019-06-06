

#include "ParticlesGPU.hpp"

#include <math.h>
#include <cmath>
#include <slmath/slmath.h>

#include <CL/cl_d3d11.h>

#include "ParticleEngine/SmoothedParticleHydrodynamics.h"
#include "ParticleEngine/ParticlesCollider.h"
#include "ParticleEngine/ParticlesSpring.h"
#include "ParticleEngine/ParticlesAccelerator.h"

#include "Utility/Utility.h"
#include "Utility/Timer.h"


typedef CL_API_ENTRY cl_mem (CL_API_CALL *PFN_clCreateFromD3D11BufferKHR)(
    cl_context context, cl_mem_flags flags, ID3D11Buffer*  buffer,
    cl_int* errcode_ret);
typedef CL_API_ENTRY cl_int (CL_API_CALL *PFN_clEnqueueAcquireD3D11ObjectsKHR)(
    cl_command_queue command_queue, cl_uint num_objects,
    const cl_mem* mem_objects, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event);
typedef CL_API_ENTRY cl_int (CL_API_CALL *PFN_clEnqueueReleaseD3D11ObjectsKHR)(
    cl_command_queue command_queue, cl_uint num_objects,
    const cl_mem* mem_objects,  cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event);

typedef CL_API_ENTRY cl_int (CL_API_CALL *PFN_clGetDeviceIDsFromD3D11KHR)(
    cl_platform_id m_PlatformId,
    cl_d3d11_device_source_khr d3d_device_source,
    void *d3d_object,
    cl_d3d11_device_set_khr d3d_device_set,
    cl_uint num_entries,
    cl_device_id *devices,
    cl_uint *num_devices);


static PFN_clCreateFromD3D11BufferKHR pfn_clCreateFromD3D11BufferKHR = NULL;;
static PFN_clEnqueueAcquireD3D11ObjectsKHR pfn_clEnqueueAcquireD3D11ObjectsKHR = NULL;
static PFN_clEnqueueReleaseD3D11ObjectsKHR pfn_clEnqueueReleaseD3D11ObjectsKHR = NULL;
static PFN_clGetDeviceIDsFromD3D11KHR pfn_clGetDeviceIDsFromD3D11KHR = NULL;


// Init extension function pointers
#define INIT_CL_EXT_FCN_PTR(platform, name) \
    if(!pfn_##name) { \
        pfn_##name = (PFN_##name) \
        clGetExtensionFunctionAddressForPlatform(platform, #name); \
        if(!pfn_##name) { \
            std::cout << "Cannot get pointer to ext. fcn. " #name << std::endl; \
            return SDK_FAILURE; \
        } \
    }


void ParticlesGPU::SetClothCount(int clothCount)
{
    m_ClothCount = clothCount;
}

void ParticlesGPU::SetIsUsingCPU(bool isUsingCPU)
{
    m_IsUsingCPU = isUsingCPU;
}

bool ParticlesGPU::IsUsingCPU() const
{
    return m_IsUsingCPU;
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

void ParticlesGPU::UpdateOutputSPH()
{
    #ifdef DEBUG
        for (int i = 0; i < m_ParticlesCount; i++)
        {
            assert(m_Density[i] > 0.0f);
        }
    #endif // DEBUG
}


void ParticlesGPU::UpdateInputCollision(slmath::vec4 *positions, slmath::vec4 *previousPositions, int particlesCount, 
                                        const Sphere *spheres, int spheresCount, 
                                        const Aabb *aabbs, int aabbsCount)
{
    assert( spheresCount <= MAX_SHAPES_COUNT);
    assert( aabbsCount <= MAX_SHAPES_COUNT);
    UNUSED_PARAMETER(particlesCount);
    assert(particlesCount == m_ParticlesCount);

    m_Positions         = reinterpret_cast<cl_float4*>(positions);
    m_PreviousPositions = reinterpret_cast<cl_float4*>(previousPositions);
    m_Spheres           = reinterpret_cast<const cl_float4*>(spheres);
    m_Aabbs             = reinterpret_cast<const cl_float4*>(aabbs);

    m_ShapesCount.s[0] = spheresCount;
    m_ShapesCount.s[1] = 2 * aabbsCount;
    m_ShapesCount.s[2] = 0;
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


int ParticlesGPU::setupCL(ID3D11Device * d3D11Device)
{
    m_ShapesCount.s[0] = MAX_SHAPES_COUNT;
    m_ShapesCount.s[1] = 2 * MAX_SHAPES_COUNT;

    cl_int status = 0;
    cl_device_type dType;

    dType = CL_DEVICE_TYPE_GPU;
    if(m_IsUsingCPU)
    {
        dType = CL_DEVICE_TYPE_CPU;
    }
    
    /*
     * Have a look at the available platforms and pick either
     * the AMD one if available or a reasonable default.
     */
    m_PlatformId = NULL;
    {
        cl_uint numPlatforms;
        cl_int status = clGetPlatformIDs(0, NULL, &numPlatforms);
        CHECK_OPENCL_ERROR(status, "clGetPlatformIDs failed.");

        if (0 < numPlatforms) 
        {
            cl_platform_id* platforms = new cl_platform_id[numPlatforms];
            status = clGetPlatformIDs(numPlatforms, platforms, NULL);
            CHECK_OPENCL_ERROR(status, "clGetPlatformIDs failed.");

            char platformName[100];
            for (unsigned i = 0; i < numPlatforms; ++i) 
            {
                status = clGetPlatformInfo(platforms[i],
                                            CL_PLATFORM_VENDOR,
                                            sizeof(platformName),
                                            platformName,
                                            NULL);
				CHECK_OPENCL_ERROR(status, "clGetPlatformInfo failed.");

                m_PlatformId = platforms[i];
                if (!strcmp(platformName, "Advanced Micro Devices, Inc.")) 
                {
                    break;
                }
            }
            std::cout << "Platform found : " << platformName << "\n";

            delete[] platforms;
        }

        CHECK_OPENCL_ERROR(int(m_PlatformId == NULL), "NULL platform found\n");
    }

    // Platform info
    char platrformInfo[1024];
    size_t param_value_size_ret = 0;
    clGetPlatformInfo(  m_PlatformId,  
        CL_PLATFORM_EXTENSIONS,  
        1024,  
        platrformInfo,  
        &param_value_size_ret);

    if(strstr(platrformInfo, "cl_khr_d3d11_sharing") == NULL)
    {
        CHECK_OPENCL_ERROR(SDK_FAILURE, "Platform does not support cl_khr_d3d11_sharing extension!");
    }


    /*
     * If we could find our platform, use it. Otherwise use just available platform.
     */
    cl_context_properties cps[3] = 
    {
        CL_CONTEXT_PLATFORM, 
        (cl_context_properties)m_PlatformId, 
        0
    };

    m_Context = clCreateContextFromType(
                  cps,
                  dType,
                  NULL,
                  NULL,
                  &status);
    CHECK_OPENCL_ERROR( status, "clCreateContextFromType failed.");


    if (d3D11Device == NULL || m_IsUsingInteroperability == false)
    {
        /* First, get the size of device list data */
        size_t deviceListSize = 0;
	    int status = 0;
	    status = clGetContextInfo(
                     m_Context, 
                     CL_CONTEXT_DEVICES, 
                     0, 
                     NULL, 
                     &deviceListSize);
        CHECK_OPENCL_ERROR(status, "clGetContextInfo failed.");

        unsigned int deviceCount = (int)(deviceListSize / sizeof(cl_device_id));
        CHECK_OPENCL_ERROR(int(deviceId >= deviceCount), "Invalid Device Selected");
        

        /**
	     * Now allocate memory for device list based on the size we got earlier
	     * Note that this memory is allocated to a pointer which is a argument
	     * so it must not be deleted inside this function. The Sample implementer
	     * has to delete the devices pointer in the host code at clean up
	     */

        (m_Devices) = (cl_device_id *)malloc(deviceListSize);
        CHECK_ALLOCATION(m_Devices, "Failed to allocate memory (devices).");

        /* Now, get the device list data */
        status = clGetContextInfo(m_Context, 
                     CL_CONTEXT_DEVICES, 
                     deviceListSize,
                     m_Devices,
                     NULL);
        CHECK_OPENCL_ERROR(status, "clGetGetContextInfo failed.");
    }
    else
    {

    
        INIT_CL_EXT_FCN_PTR(m_PlatformId, clGetDeviceIDsFromD3D11KHR);

        // Get device from DirectX11
        cl_uint devicesCount = 0;
        status = pfn_clGetDeviceIDsFromD3D11KHR (  m_PlatformId,  
                                      CL_D3D11_DEVICE_KHR,  
                                      d3D11Device,  
                                      CL_PREFERRED_DEVICES_FOR_D3D11_KHR,  
                                      0,  
                                      NULL,  
                                      &devicesCount);
        CHECK_OPENCL_ERROR(status, "clGetDeviceIDsFromD3D11KHR failed.");

        m_Devices = new cl_device_id[devicesCount];
        CHECK_ALLOCATION(m_Devices, "Failed to allocate memory (devices).");


        status = pfn_clGetDeviceIDsFromD3D11KHR (  m_PlatformId,  
                                      CL_D3D11_DEVICE_KHR,  
                                      d3D11Device,  
                                      CL_PREFERRED_DEVICES_FOR_D3D11_KHR,  
                                      devicesCount,  
                                      m_Devices,
                                      NULL);
        CHECK_OPENCL_ERROR(status, "clGetDeviceIDsFromD3D11KHR failed.");


    }


    {
        // The block is to move the declaration of prop closer to its use
        cl_command_queue_properties prop = 0;
        m_CommandQueue = clCreateCommandQueue(
                m_Context, 
                m_Devices[deviceId], 
                prop,
                &status);
        CHECK_OPENCL_ERROR( status, "clCreateCommandQueue failed.");
    }


    //Set device info of given cl_device_id
    cl_int retValue = m_DeviceInfo.setDeviceInfo(m_Devices[deviceId]);
    CHECK_ERROR(retValue, SDK_SUCCESS, "SDKDeviceInfo::setDeviceInfo() failed");

    // Check particular extension
   /* if(!strstr(m_DeviceInfo.extensions, "cl_khr_byte_addressable_store"))
    {
        byteRWSupport = false;
        OPENCL_EXPECTED_ERROR("Device does not support cl_khr_byte_addressable_store extension!\n");
    }*/

    if (d3D11Device != NULL)
    {
        if(strstr(m_DeviceInfo.extensions, "cl_khr_d3d11_sharing") == NULL)
        {
           DEBUG_OUT("Device does not support cl_khr_d3d11_sharing extension!\n");
        }
    }

    return SDK_SUCCESS;
}

int ParticlesGPU::CreateBuffers(ID3D11Buffer *d3D11buffer /*= NULL*/)
{
    cl_int status = 0;

    // Set Presistent memory only for AMD m_PlatformId
    cl_mem_flags inMemFlags = CL_MEM_READ_ONLY;
    if(isAmdPlatform())
        inMemFlags |= CL_MEM_USE_PERSISTENT_MEM_AMD;


    // Input buffer
    if (d3D11buffer == NULL || m_IsUsingInteroperability == false)
    {
        m_PositionsBuffer = clCreateBuffer( m_Context,
                                            CL_MEM_READ_WRITE | CL_MEM_USE_PERSISTENT_MEM_AMD,
                                            sizeof(cl_float4) * m_ParticlesCount,
                                            NULL,
                                            &status);
        CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (m_PositionsBuffer)");
    }
    else
    {

        INIT_CL_EXT_FCN_PTR(m_PlatformId, clCreateFromD3D11BufferKHR);
        /*INIT_CL_EXT_FCN_PTR(m_PlatformId, clEnqueueAcquireD3D11ObjectsKHR);
        INIT_CL_EXT_FCN_PTR(m_PlatformId, clEnqueueReleaseD3D11ObjectsKHR);*/
        m_PositionsBuffer = pfn_clCreateFromD3D11BufferKHR (   m_Context,  
                                                            CL_MEM_READ_WRITE,
                                                            d3D11buffer,  
                                                            &status);
        CHECK_OPENCL_ERROR(status, "clCreateFromD3D11BufferKHR failed. (m_PositionsBuffer)");
    }
    


     // Input buffer
    m_PreviousPositionsBuffer = clCreateBuffer(
        m_Context,
        CL_MEM_READ_WRITE,
        sizeof(cl_float4) * m_ParticlesCount,
        NULL,
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (m_PreviousPositionsBuffer)");

    // Density double buffer
    m_PreviousDensityBuffer  = clCreateBuffer(
        m_Context,
        CL_MEM_READ_WRITE,
        sizeof(cl_float) * m_ParticlesCount,
        NULL,
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (m_PreviousDensityBuffer)");

    // Density double buffer
    m_OutputDensityBuffer  = clCreateBuffer(
        m_Context,
        CL_MEM_READ_WRITE,
        sizeof(cl_float) * m_ParticlesCount,
        NULL,
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (m_OutputDensityBuffer)");

    // Input buffer for SPH OutputBuffer fo CreateGrid
    m_NeighborsInfoBuffer = clCreateBuffer(
        m_Context,
        CL_MEM_READ_WRITE,
        sizeof(cl_int2) * m_ParticlesCount,
        NULL,
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (m_NeighborsInfoBuffer)");

    // Internal GPU buffer
    m_NeighborsInfoBuffer2 = clCreateBuffer(
        m_Context,
        CL_MEM_READ_WRITE,
        sizeof(cl_int2) * m_ParticlesCount,
        NULL,
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (m_NeighborsInfoBuffer2)");

    // Internal GPU buffer
    m_RealGridBuffer = clCreateBuffer(
        m_Context,
        CL_MEM_READ_WRITE,
        sizeof(cl_int) * m_RealGridSize,
        NULL,
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (m_NeighborsInfoBuffer2)");

    // Shapes for collision
    m_SpheresBuffer = clCreateBuffer(
        m_Context,
        inMemFlags,
        sizeof(cl_float4) * m_ShapesCount.s[0],
        NULL,
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (m_SpheresBuffer)");

    m_AabbsBuffer = clCreateBuffer(
        m_Context,
        inMemFlags,
        sizeof(cl_float4) * m_ShapesCount.s[1],
        NULL,
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (m_AabbsBuffer)");


    m_SphParametersBuffer = clCreateBuffer( m_Context,
                                            inMemFlags,
                                            sizeof(SphParameters),
                                            NULL,
                                            &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (m_SphParametersBuffer)");

    // Spring
    if (m_SpringsCount > 0)
    {
        m_SpringsBuffer = clCreateBuffer(   m_Context,
                                            inMemFlags,
                                            sizeof(Spring) * m_SpringsCount,
                                            NULL,
                                            &status);
        CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (m_SpringsBuffer)");
    }
    else
    {
        m_SpringsBuffer = NULL;
    }

    if (m_AcceleratorsCount > 0)
    {
    
        m_AcceleratorsBuffer = clCreateBuffer( m_Context,
                                                inMemFlags,
                                                sizeof(Accelerator) * m_AcceleratorsCount,
                                                NULL,
                                                &status);
        CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (m_AcceleratorsBuffer)");
    }
    else 
    {
        m_AcceleratorsBuffer = NULL;
    }


    m_GridInfoBuffer = clCreateBuffer(
        m_Context,
        CL_MEM_READ_WRITE,
        sizeof(cl_int4) ,
        NULL,
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (m_GridInfoBuffer)");

    m_MinMaxBuffer  = clCreateBuffer(
        m_Context,
        CL_MEM_READ_WRITE,
        2 * sizeof(cl_float4),
        NULL,
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (m_MinMaxBuffer)");
    
    // Output positions buffer
    m_OutputBuffer = clCreateBuffer(m_Context, 
                                    CL_MEM_READ_WRITE,
                                    sizeof(cl_float4) * m_ParticlesCount,
                                    NULL, 
                                    &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (m_OutputBuffer)");


    return SDK_SUCCESS;
}


int ParticlesGPU::CreeateKernels()
{

    cl_int status = 0;

    // create a CL m_Program using the kernel source 
    streamsdk::buildProgramData buildData;
    buildData.kernelName = std::string("openCL\\ParticlesGPU_Kernels.cl");
    buildData.devices = m_Devices;
    buildData.deviceId = deviceId;
    buildData.flagsStr = std::string("");

    if(isLoadBinaryEnabled())
        buildData.binaryName = std::string(loadBinary.c_str());

    if(isComplierFlagsSpecified())
        buildData.flagsFileName = std::string(flags.c_str());

    int retValue = sampleCommon->buildOpenCLProgram(m_Program, m_Context, buildData);
    CHECK_ERROR(retValue, SDK_SUCCESS, "sampleCommon::buildOpenCLProgram() failed");

    // get a kernel object handle for a kernel with the given name
    m_CreateGridKernel = clCreateKernel(m_Program, "CreateGrid", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");

    status = m_CreateGridKernelInfo.setKernelWorkGroupInfo(m_CreateGridKernel, m_Devices[deviceId]);
    CHECK_ERROR(status, SDK_SUCCESS, "m_CreateGridKernelInfo.setKernelWorkGroupInfo() failed");

    // get a kernel object handle for a kernel with the given name
    m_SortLocalKernel = clCreateKernel(m_Program, "SortLocalGridCells", &status);
    CHECK_OPENCL_ERROR(status, "m_SortLocalKernel failed.");

    status = m_SortLocalKernelInfo.setKernelWorkGroupInfo(m_SortLocalKernel, m_Devices[deviceId]);
    CHECK_ERROR(status, SDK_SUCCESS, "m_SortLocalKernelInfo.setKernelWorkGroupInfo() failed");

    // get a kernel object handle for a kernel with the given name
    m_MergeSortKernel = clCreateKernel(m_Program, "MergeSortGridCells", &status);
    CHECK_OPENCL_ERROR(status, "m_MergeSortKernel failed.");

    status = m_MergeSortKernelInfo.setKernelWorkGroupInfo(m_MergeSortKernel, m_Devices[deviceId]);
    CHECK_ERROR(status, SDK_SUCCESS, "m_MergeSortKernelInfo.setKernelWorkGroupInfo() failed");

    // get a kernel object handle for a kernel with the given name
    m_MinMaxKernel = clCreateKernel(m_Program, "ComputeMinMax", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");

    status = m_MinKernelInfo.setKernelWorkGroupInfo(m_MinMaxKernel, m_Devices[deviceId]);
    CHECK_ERROR(status, SDK_SUCCESS, "m_MinKernelInfo.setKernelWorkGroupInfo() failed");

    // get a kernel object handle for a kernel with the given name
    m_ClearRealGridKernel = clCreateKernel(m_Program, "ClearRealGrid", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");

    status = m_ClearRealGridKernelInfo.setKernelWorkGroupInfo(m_ClearRealGridKernel, m_Devices[deviceId]);
    CHECK_ERROR(status, SDK_SUCCESS, "m_ClearRealGridKernelInfo.setKernelWorkGroupInfo() failed");

    // get a kernel object handle for a kernel with the given name
    m_RealGridKernel = clCreateKernel(m_Program, "SetRealGrid", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");

    status = m_RealGridKernelInfo.setKernelWorkGroupInfo(m_RealGridKernel, m_Devices[deviceId]);
    CHECK_ERROR(status, SDK_SUCCESS, "m_RealGridKernelInfo.setKernelWorkGroupInfo() failed");

    // get a kernel object handle for a kernel with the given name
    m_SPHIntegrateKernel = clCreateKernel(m_Program, "ComputeSPH", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");

    status = m_SPHIntegrateKernelInfo.setKernelWorkGroupInfo(m_SPHIntegrateKernel, m_Devices[deviceId]);
    CHECK_ERROR(status, SDK_SUCCESS, "m_SPHIntegrateKernelInfo.setKernelWorkGroupInfo() failed");

    // get a kernel object handle for a kernel with the given name
    m_CollisionKernel = clCreateKernel(m_Program, "ComputeCollision", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");

    status = m_CollisionKernelInfo.setKernelWorkGroupInfo(m_CollisionKernel, m_Devices[deviceId]);
    CHECK_ERROR(status, SDK_SUCCESS, "m_CollisionKernelInfo.setKernelWorkGroupInfo() failed");

    // get a kernel object handle for a kernel with the given name
    m_SpringKernel = clCreateKernel(m_Program, "ComputeSpring", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");

    status = m_SpringKernelInfo.setKernelWorkGroupInfo(m_SpringKernel, m_Devices[deviceId]);
    CHECK_ERROR(status, SDK_SUCCESS, "m_SpringKernelInfo.setKernelWorkGroupInfo() failed");

    // get a kernel object handle for a kernel with the given name
    m_AcceleratorKernel = clCreateKernel(m_Program, "ComputeAccelerator", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");

    status = m_AcceleratorKernelInfo.setKernelWorkGroupInfo(m_AcceleratorKernel, m_Devices[deviceId]);
    CHECK_ERROR(status, SDK_SUCCESS, "m_AcceleratorKernelInfo.setKernelWorkGroupInfo() failed");

    return SDK_SUCCESS;
}

int ParticlesGPU::InitializeCreateGrid()
{
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

    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (m_PositionsBuffer)");

     status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    // No clFinish command queue because there is one before running kenel !

    return SDK_SUCCESS;

}

int ParticlesGPU::CreateGrid()
{

    Timer::GetInstance()->StartTimerProfile();
    // This kernel won't support les than 10 work items
    assert(m_LocalThreads > 10);
    cl_int status;


    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFinish failed.");

    // Find the minimum and the maximum position

    
    // Setup kernel arguments
    status = clSetKernelArg(m_MinMaxKernel, 
                            0, 
                            sizeof(cl_mem), 
                            (void *)&m_PositionsBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_PositionsBuffer)");

     // local memory
    status = clSetKernelArg(m_MinMaxKernel,
                            1,
                            2 * m_LocalThreads * 4 * sizeof(float),
                            NULL);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (localPos)");

    // global memory
    status = clSetKernelArg(m_MinMaxKernel, 
                            2,
                            sizeof(cl_mem), 
                            (void *)&m_MinMaxBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_MinMaxBuffer)");


    if (m_MinKernelInfo.localMemoryUsed > m_DeviceInfo.localMemSize)
    {
        DEBUG_OUT("Unsupported: Insufficient"
            "local memory on device." );
        return SDK_FAILURE;
    }

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
    CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFinish failed.");

    // Create grid

    // Setup kernel arguments
    status = clSetKernelArg(m_CreateGridKernel, 
                            0, 
                            sizeof(cl_mem), 
                            (void *)&m_PositionsBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_PositionsBuffer)");

     
    // global memory
    status = clSetKernelArg(m_CreateGridKernel, 
                            1,
                            sizeof(cl_mem), 
                            (void *)&m_MinMaxBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_MinMaxBuffer)");

    // Neighbors info
    status = clSetKernelArg(m_CreateGridKernel,
                            2,
                            sizeof(cl_mem), 
                            (void *)&m_NeighborsInfoBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_NeighborsInfoBuffer)");

    status = clSetKernelArg(m_CreateGridKernel,
                            3,
                            sizeof(cl_mem), 
                            (void *)&m_GridInfoBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_GridInfoBuffer)");

    if(m_CreateGridKernelInfo.localMemoryUsed > m_DeviceInfo.localMemSize)
    {
        DEBUG_OUT("Unsupported: Insufficient"
            "local memory on device." );
        return SDK_FAILURE;
    }   

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
    CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFinish failed.");

    Timer::GetInstance()->StopTimerProfile("Create Grid : Grid Creation");

    return SDK_SUCCESS;
}

void ParticlesGPU::TestSort()
{
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

    assert(status == SDK_SUCCESS);

    status = clFlush(m_CommandQueue);
    assert(status == SDK_SUCCESS);

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    assert(status == SDK_SUCCESS);

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
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_NeighborsInfoBuffer)");

    // Neighbors info buffer
    status = clSetKernelArg(m_SortLocalKernel,
                            1,
                            sizeof(cl_mem), 
                            (void *)&m_NeighborsInfoBuffer2); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_NeighborsInfoBuffer 2 )");


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
    CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFinish failed.");


    // Selection Merge sort on GPU

    // Neighbors info
    status = clSetKernelArg(m_MergeSortKernel,
                            0,
                            sizeof(cl_mem), 
                            (void *)&m_NeighborsInfoBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_NeighborsInfoBuffer)");

    // Neighbors info buffer
    status = clSetKernelArg(m_MergeSortKernel,
                            1,
                            sizeof(cl_mem), 
                            (void *)&m_NeighborsInfoBuffer2); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_NeighborsInfoBuffer 2 )");


    // Counters and / or for selection
    status = clSetKernelArg(m_MergeSortKernel,
                            2,
                            m_LocalThreads * sizeof(cl_int2), 
                            NULL); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (counters)");


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
    CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFinish failed.");



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
    CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFinish failed.");

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
        CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");

        status = clFlush(m_CommandQueue);
        CHECK_OPENCL_ERROR(status, "clFlush failed.");
        // All the command in queue must be finihed before continuing to process
        status = clFinish(m_CommandQueue);
        CHECK_OPENCL_ERROR(status, "clFinish failed.");


        CheckOutputGrid();
    #endif // DEBUG

    return SDK_SUCCESS;
}


int ParticlesGPU::runKernelCreateGrid()
{
    int status = CreateGrid();
    CHECK_OPENCL_ERROR(status, "Grid Creation.");
    status = SortGrid();
    CHECK_OPENCL_ERROR(status, "Sort Grid.");
    status = runKernelClearRealGrid();
    CHECK_OPENCL_ERROR(status, "Clear real grid.");
    status = runKernelRealGrid();
    CHECK_OPENCL_ERROR(status, "Create real grid.");
    return SDK_SUCCESS;
}


int ParticlesGPU::runKernelMinTest()
{
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
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (m_PositionsBuffer)");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFinish failed.");



    // Setup kernel arguments
    status = clSetKernelArg(m_MinMaxKernel, 
                            0, 
                            sizeof(cl_mem), 
                            (void *)&m_PositionsBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_PositionsBuffer)");

     // local memory
    status = clSetKernelArg(m_MinMaxKernel,
                            1,
                            2 * m_LocalThreads * 4 * sizeof(float),
                            NULL);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (localPos)");

    // global memory
    status = clSetKernelArg(m_MinMaxKernel, 
                            2,
                            sizeof(cl_mem), 
                            (void *)&m_MinMaxBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_MinMaxBuffer)");


    if(m_MinKernelInfo.localMemoryUsed > m_DeviceInfo.localMemSize)
    {
        DEBUG_OUT("Unsupported: Insufficient"
            "local memory on device." );
        return SDK_FAILURE;
    }

    /* 
    * Enqueue a kernel run call.
    */
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
    CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFinish failed.");
    
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
    CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFinish failed.");

    
    CheckOutputMinMax();
    
#endif // DEBUG
    return SDK_SUCCESS;
}

int *grid = new int[128*128*128];

int ParticlesGPU::runKernelClearRealGrid()
{
    cl_int status;
    cl_event ndrEvt;

    // m_RealGridKernel
    status = clSetKernelArg(m_ClearRealGridKernel, 
                            0, 
                            sizeof(cl_mem), 
                            (void *)&m_RealGridBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_RealGridBuffer)");

    /* 
    * Enqueue a kernel run call.
    */
    status = clEnqueueNDRangeKernel(
        m_CommandQueue,
        m_ClearRealGridKernel,
        1,
        NULL,
        &m_GlobalThreads,
        &m_LocalThreads,
        0,
        NULL,
        &ndrEvt);
    CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFinish failed.");


    // Enqueue the results to application pointer
    cl_event readEvt;
    status = clEnqueueReadBuffer(
        m_CommandQueue, 
        m_RealGridBuffer, 
        CL_FALSE,
        0,
        sizeof(cl_int) * m_RealGridSize,
        grid,
        0,
        NULL,
        &readEvt);
    CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFinish failed.");

    
    return SDK_SUCCESS;
}


int ParticlesGPU::runKernelRealGrid()
{
    cl_int status;
    cl_event ndrEvt;

    // m_NeighborsInfoBuffer
    status = clSetKernelArg(m_RealGridKernel, 
                            0, 
                            sizeof(cl_mem), 
                            (void *)&m_NeighborsInfoBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_NeighborsInfoBuffer)");

    // m_RealGridKernel
    status = clSetKernelArg(m_RealGridKernel, 
                            1, 
                            sizeof(cl_mem), 
                            (void *)&m_RealGridBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_RealGridBuffer)");

    /* 
    * Enqueue a kernel run call.
    */
    status = clEnqueueNDRangeKernel(
        m_CommandQueue,
        m_RealGridKernel,
        1,
        NULL,
        &m_GlobalThreads,
        &m_LocalThreads,
        0,
        NULL,
        &ndrEvt);
    CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFinish failed.");


    // Enqueue the results to application pointer
    cl_event readEvt;
    status = clEnqueueReadBuffer(
        m_CommandQueue, 
        m_RealGridBuffer, 
        CL_FALSE,
        0,
        sizeof(cl_int) * m_RealGridSize,
        grid,
        0,
        NULL,
        &readEvt);
    CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFinish failed.");


    //#ifdef DEBUG
    //    for (int i = 0; i < m_ParticlesCount; i++)
    //        for (int j = i + 1; j < m_ParticlesCount; j++)
    //        {
    //            if (m_NeighborsInfo[i].s[1] == m_NeighborsInfo[j].s[1])
    //            {
    //                int gridIndex = m_NeighborsInfo[i].s[1];
    //                assert (i < j);
    //                assert (grid[gridIndex] != j);
    //            }
    //        }
    //#endif // DEBUG


    return SDK_SUCCESS;
}


int ParticlesGPU::InitializeKernelSPH()
{
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
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (m_PositionsBuffer)");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

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
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (m_PreviousPositionsBuffer)");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");


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
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (m_PreviousDensityBuffer)");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");


    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFinish failed.");

    return SDK_SUCCESS;
}

int ParticlesGPU::runKernelSPH()
{
   
    cl_int status;


    // Enqueue write from m_GridInfo to m_GridInfoBuffer
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
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (m_GridInfoBuffer)");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

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
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (m_GridInfoBuffer)");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFinish failed.");

    // Setup kernel arguments
    status = clSetKernelArg(m_SPHIntegrateKernel, 
                            0, 
                            sizeof(cl_mem), 
                            (void *)&m_PositionsBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_PositionsBuffer)");

    status = clSetKernelArg(m_SPHIntegrateKernel, 
                            1, 
                            sizeof(cl_mem), 
                            (void *)&m_PreviousPositionsBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_PreviousPositionsBuffer)");

    status = clSetKernelArg(m_SPHIntegrateKernel, 
                            2, 
                            sizeof(cl_mem), 
                            (void *)&m_NeighborsInfoBuffer2); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_NeighborsInfoBuffer2)");

    status = clSetKernelArg(m_SPHIntegrateKernel, 
                            3, 
                            sizeof(cl_mem), 
                            (void *)&m_RealGridBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_RealGridBuffer)");

    status = clSetKernelArg(m_SPHIntegrateKernel, 
                            4, 
                            sizeof(cl_mem), 
                            (void *)&m_GridInfoBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_GridInfo)");

    status = clSetKernelArg(m_SPHIntegrateKernel, 
                            5, 
                            sizeof(cl_mem), 

                            (void *)&m_SphParametersBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_SphParameters)");

    status = clSetKernelArg(m_SPHIntegrateKernel, 
                            6, 
                            sizeof(cl_mem), 

                            (void *)&m_OutputBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_OutputBuffer)");


    cl_int firstIndex = m_SwapBufferSPH ? 8 : 7;
    cl_int secondIndex = m_SwapBufferSPH ? 7 : 8;

    status = clSetKernelArg(m_SPHIntegrateKernel, 
                            firstIndex, 
                            sizeof(cl_mem), 
                            (void *)&m_PreviousDensityBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_PreviousDensityBuffer)");

    status = clSetKernelArg(m_SPHIntegrateKernel, 
                            secondIndex, 
                            sizeof(cl_mem), 
                            (void *)&m_OutputDensityBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_OutputDensityBuffer)");

    // Swap buffer next time
    m_SwapBufferSPH = ! m_SwapBufferSPH;


    if(m_SPHIntegrateKernelInfo.localMemoryUsed > m_DeviceInfo.localMemSize)
    {
        DEBUG_OUT("Unsupported: Insufficient"
            "local memory on device." );
        return SDK_FAILURE;
    }


    /* 
    * Enqueue a kernel run call.
    */
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
    CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFinish failed.");

#ifdef DEBUG
    // Enqueue write from m_Density to m_PreviousDensityBuffer
    cl_event readEvt;
    status = clEnqueueReadBuffer(
        m_CommandQueue, 
        m_OutputDensityBuffer, 
        CL_FALSE,
        0,
        sizeof(cl_float) * m_ParticlesCount,
        m_Density,
        0,
        NULL,
        &readEvt);
    CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFinish failed.");
    
     UpdateOutputSPH();
#endif // DEBUG

    return SDK_SUCCESS;
}

int ParticlesGPU::InitializeKernelCollision()
{
    cl_int status;

    // Enqueue write from m_Positions to m_PositionsBuffer
    cl_event writeEvt;
    status = clEnqueueWriteBuffer(m_CommandQueue, 
                                  m_OutputBuffer, 
                                  CL_FALSE,
                                  0, 
                                  sizeof(cl_float4) * m_ParticlesCount,
                                  m_Positions, 
                                  0, 
                                  NULL, 
                                  &writeEvt);
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (m_PositionsBuffer)");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

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
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (m_PreviousPositionsBuffer)");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    // No clFinish command queue because there is one before running kenel !

    return SDK_SUCCESS;

}


int ParticlesGPU::runKernelCollision()
{
    cl_int status;

    // Enqueue write from m_Spheres to m_SpheresBuffer
    cl_event writeEvt2;
    status = clEnqueueWriteBuffer(m_CommandQueue, 
                                  m_SpheresBuffer, 
                                  CL_FALSE,
                                  0, 
                                  sizeof(cl_float4) * m_ShapesCount.s[0],
                                  m_Spheres, 
                                  0, 
                                  NULL, 
                                  &writeEvt2);
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (m_Spheres)");

     status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");


    // Enqueue write from m_Aabbs to m_AabbsBuffer
    cl_event writeEvt3;
    status = clEnqueueWriteBuffer(m_CommandQueue, 
                                  m_AabbsBuffer,
                                  CL_FALSE,
                                  0, 
                                  sizeof(cl_float4) * m_ShapesCount.s[1],
                                  m_Aabbs, 
                                  0, 
                                  NULL, 
                                  &writeEvt3);
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (m_AabbsBuffer)");

     status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFinish failed.");

    // Setup kernel arguments
    status = clSetKernelArg(m_CollisionKernel, 
                            0, 
                            sizeof(cl_mem), 
                            (void *)&m_OutputBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_PositionsBuffer)");

    status = clSetKernelArg(m_CollisionKernel, 
                            1, 
                            sizeof(cl_mem), 
                            (void *)&m_PreviousPositionsBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_PreviousPositionsBuffer)");

    status = clSetKernelArg(m_CollisionKernel, 
                            2,
                            sizeof(cl_mem), 
                            (void *)&m_SpheresBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_SpheresBuffer)");

    status = clSetKernelArg(m_CollisionKernel, 
                            3, 
                            sizeof(cl_mem), 
                            (void *)&m_AabbsBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_AabbsBuffer)");

    status = clSetKernelArg(m_CollisionKernel, 
                            4, 
                            sizeof(cl_int4), 
                            (void *)&m_ShapesCount);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_ShapesCount)");

    status = clSetKernelArg(m_CollisionKernel, 
                            5, 
                            sizeof(cl_mem), 
                            (void *)&m_PositionsBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_PositionsBuffer)");


    if(m_CollisionKernelInfo.localMemoryUsed > m_DeviceInfo.localMemSize)
    {
        DEBUG_OUT("Unsupported: Insufficient"
            "local memory on device." );
        return SDK_FAILURE;
    }

    /* 
    * Enqueue a kernel run call.
    */
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
    CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    // All the command in queue must be finihed before continuing to process
    status = clFinish(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFinish failed.");

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
        CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");

        status = clFlush(m_CommandQueue);
        CHECK_OPENCL_ERROR(status, "clFlush failed.");


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
        CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");

        status = clFlush(m_CommandQueue);
        CHECK_OPENCL_ERROR(status, "clFlush failed.");

        // All the command in queue must be finihed before continuing to process
        status = clFinish(m_CommandQueue);
        CHECK_OPENCL_ERROR(status, "clFinish failed.");
    }

    return SDK_SUCCESS;
}


int ParticlesGPU::InitializeKernelSpring()
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
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (m_PositionsBuffer)");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

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
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (m_Springs)");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    // No clFinish command queue because there is one before running kenel !

    return SDK_SUCCESS;
}

int ParticlesGPU::runKernelSpring()
{
    cl_int status;


    // All buffer must be written before sending args
    status = clFinish(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFinish failed.");
    
    // Setup kernel arguments
    status = clSetKernelArg(m_SpringKernel, 
                            0, 
                            sizeof(cl_mem), 
                            (void *)&m_PositionsBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_PositionsBuffer)");

    status = clSetKernelArg(m_SpringKernel, 
                            1, 
                            sizeof(cl_mem), 
                            (void *)&m_SpringsBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_SpringsBuffer)");

    status = clSetKernelArg(m_SpringKernel, 
                            2, 
                            sizeof(cl_float4) * m_ParticlesCount / m_ClothCount, // particles by cloth or work item 
                            NULL);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (Local memory)");

    status = clSetKernelArg(m_SpringKernel, 
                            3, 
                            sizeof(cl_mem), 
                            (void *)&m_OutputBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_PositionsBuffer)");
    

    if(m_SpringKernelInfo.localMemoryUsed > m_DeviceInfo.localMemSize)
    {
        DEBUG_OUT("Unsupported: Insufficient"
            "local memory on device." );
        return SDK_FAILURE;
    }

    /* 
    * Enqueue a kernel run call.
    */
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
    CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    // Kernel must be finished before reading data
    status = clFinish(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFinish failed.");
    

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
        CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");

        status = clFlush(m_CommandQueue);
        CHECK_OPENCL_ERROR(status, "clFlush failed.");

         // Block until all output data are ready
        status = clFinish(m_CommandQueue);
        CHECK_OPENCL_ERROR(status, "clFinish failed.");
    }

    return SDK_SUCCESS;
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
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (m_PositionsBuffer)");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");


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
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (m_PreviousPositionsBuffer)");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    // No clFinish command queue because there is one before running kenel !

    return SDK_SUCCESS;
}

int ParticlesGPU::runKernelAccelerator()
{
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
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (m_AcceleratorsBuffer)");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");


    // All buffer must be written before sending args
    status = clFinish(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFinish failed.");
    
    // Setup kernel arguments

    
    status = clSetKernelArg(m_AcceleratorKernel, 
                            0, 
                            sizeof(cl_mem), 
                            (void *)&m_PositionsBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_PositionsBuffer)");

    status = clSetKernelArg(m_AcceleratorKernel, 
                            1, 
                            sizeof(cl_mem), 
                            (void *)&m_PreviousPositionsBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_PreviousPositionsBuffer)");

    status = clSetKernelArg(m_AcceleratorKernel, 
                            2, 
                            sizeof(cl_mem), 
                            (void *)&m_AcceleratorsBuffer); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_AcceleratorsBuffer)");

    status = clSetKernelArg(m_AcceleratorKernel, 
                            3, 
                            sizeof(cl_int), 
                            (void *)&m_AcceleratorsCount); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_AcceleratorsCount)");

    
     status = clSetKernelArg(m_AcceleratorKernel, 
                            4,
                            sizeof(cl_mem),
                            (void *)&m_OutputBuffer);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (m_OutputBuffer)");
    if(m_AcceleratorKernelInfo.localMemoryUsed > m_DeviceInfo.localMemSize)
    {
        DEBUG_OUT("Unsupported: Insufficient"
            "local memory on device." );
        return SDK_FAILURE;
    }

    /* 
    * Enqueue a kernel run call.
    */
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
    CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

    status = clFlush(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    // Kernel must be finished before reading data
    status = clFinish(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clFinish failed.");

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
            m_PreviousPositions,
            0,
            NULL,
            &readEvt);
        CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");

        status = clFlush(m_CommandQueue);
        CHECK_OPENCL_ERROR(status, "clFlush failed.");

         // Block until all output data are ready
        status = clFinish(m_CommandQueue);
        CHECK_OPENCL_ERROR(status, "clFinish failed.");
    }
   

    return SDK_SUCCESS;
}


int ParticlesGPU::setup(    int particlesCount, int springsCount, int acceleratorsCount,
                            ID3D11Device * d3D11Device /*= NULL*/, ID3D11Buffer *d3D11buffer /*= NULL*/)
{

    assert( ! (d3D11Device == NULL && d3D11buffer != NULL ) );
    m_ParticlesCount = particlesCount;

    m_SpringsCount = springsCount;

    m_AcceleratorsCount = acceleratorsCount;

    int status = setupCL(d3D11Device);
    if(status == SDK_SUCCESS)
    {
        status = CreateBuffers(d3D11buffer);
    }
    if(status == SDK_SUCCESS)
    {
        status = CreeateKernels();
    }
    
    if(status != SDK_SUCCESS)
    {
        if(status == SDK_EXPECTED_FAILURE) 
            return SDK_EXPECTED_FAILURE;

        return SDK_FAILURE;
    }

    m_GlobalThreads = sampleCommon->roundToPowerOf2<size_t>(particlesCount);
    m_LocalThreads  = std::min(sampleCommon->roundToPowerOf2<size_t>((particlesCount >> 1) + 1), m_DeviceInfo.maxWorkGroupSize);

    if (m_LocalThreads > m_DeviceInfo.maxWorkItemSizes[0] || 
        m_LocalThreads > m_DeviceInfo.maxWorkGroupSize)
    {
        DEBUG_OUT("Unsupported: Device does not support requested number of work items.\n");
        return SDK_FAILURE;
    }
    m_WorkGroupCount = m_GlobalThreads / m_LocalThreads;
    m_Counters = new int[m_WorkGroupCount];

    int workItemsCount = m_SpringsCount / 12;

    m_GlobalThreadsSpring = sampleCommon->roundToPowerOf2<size_t>(workItemsCount );
    m_LocalThreadsSpring  = std::min(m_GlobalThreadsSpring / m_ClothCount, m_DeviceInfo.maxWorkGroupSize);

    if (m_LocalThreadsSpring > m_DeviceInfo.maxWorkItemSizes[0] || 
        m_LocalThreadsSpring > m_DeviceInfo.maxWorkGroupSize)
    {
        DEBUG_OUT("Unsupported: Device does notsupport requested number of work items.\n");
        return SDK_FAILURE;
    }

    return SDK_SUCCESS;
}

int ParticlesGPU::cleanup()
{

    delete []m_Counters;

    if(!byteRWSupport)
        return SDK_SUCCESS;

    cl_int status;

    // Release openCL buffer

    status = clReleaseMemObject(m_PositionsBuffer);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(m_PositionsBuffer)");

    status = clReleaseMemObject(m_PreviousPositionsBuffer);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(m_PreviousPositionsBuffer)");

    status = clReleaseMemObject(m_PreviousDensityBuffer);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(m_PreviousDensityBuffer)");

    status = clReleaseMemObject(m_OutputDensityBuffer);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(m_OutputDensityBuffer)");

    status = clReleaseMemObject(m_OutputBuffer);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(m_OutputBuffer)");


    if (m_SpringsCount > 0)
    {
        status = clReleaseMemObject(m_SpringsBuffer);
        CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(m_SpringsBuffer)");
    }
    
    if (m_AcceleratorsCount > 0)
    {
        status = clReleaseMemObject(m_AcceleratorsBuffer);
        CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(m_AcceleratorsBuffer)");
    }

    // Releases OpenCL resources (Context, Memory etc.) 
    
    status = clReleaseKernel(m_SPHIntegrateKernel);
    CHECK_OPENCL_ERROR(status, "clReleaseKernel failed.(m_SPHIntegrateKernel)");

    status = clReleaseKernel(m_CollisionKernel);
    CHECK_OPENCL_ERROR(status, "clReleaseKernel failed.(m_CollisionKernel)");

    status = clReleaseProgram(m_Program);
    CHECK_OPENCL_ERROR(status, "clReleaseProgram failed.(m_Program)");

    status = clReleaseCommandQueue(m_CommandQueue);
    CHECK_OPENCL_ERROR(status, "clReleaseCommandQueue failed.(m_CommandQueue)");

    status = clReleaseContext(m_Context);
    CHECK_OPENCL_ERROR(status, "clReleaseContext failed.");

    FREE(m_Devices);

    return SDK_SUCCESS;
}

