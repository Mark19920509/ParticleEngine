
#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable


inline float4 AtomicMin(volatile __global float4 *source, float4 value) 
{
    union {
        float4 m_Float4;
        int    m_4Ints[4]; 
    } prevVal;

    union {
        float4 m_Float4;
        int    m_4Ints[4]; 
    } newVal;


    do {
        prevVal.m_Float4 = *source;
        newVal.m_Float4.x = min(prevVal.m_Float4.x, value.x);
    } while (atomic_cmpxchg((volatile __global int*)source + 0, prevVal.m_4Ints[0], newVal.m_4Ints[0]) != prevVal.m_4Ints[0]);

    do {
        prevVal.m_Float4 = *source;
        newVal.m_Float4.y = min(prevVal.m_Float4.y, value.y);
    } while (atomic_cmpxchg((volatile __global int*)source + 1, prevVal.m_4Ints[1], newVal.m_4Ints[1]) != prevVal.m_4Ints[1]);

    do {
        prevVal.m_Float4 = *source;
        newVal.m_Float4.z = min(prevVal.m_Float4.z, value.z);
    } while (atomic_cmpxchg((volatile __global int*)source + 2, prevVal.m_4Ints[2], newVal.m_4Ints[2]) != prevVal.m_4Ints[2]);
    
    return prevVal.m_Float4;
}

inline float4 AtomicMax(volatile __global float4 *source, float4 value) 
{
    union {
        float4 m_Float4;
        int    m_4Ints[4]; 
    } prevVal;

    union {
        float4 m_Float4;
        int    m_4Ints[4]; 
    } newVal;

  
    do {
        prevVal.m_Float4 = *source;
        newVal.m_Float4.x = max(prevVal.m_Float4.x, value.x);
    } while (atomic_cmpxchg(((volatile __global int*)source) + 0, prevVal.m_4Ints[0], newVal.m_4Ints[0]) != prevVal.m_4Ints[0]);

    do {
        prevVal.m_Float4 = *source;
        newVal.m_Float4.y = max(prevVal.m_Float4.y, value.y);
    } while (atomic_cmpxchg(((volatile __global int*)source) + 1, prevVal.m_4Ints[1], newVal.m_4Ints[1]) != prevVal.m_4Ints[1]);

    do {
        prevVal.m_Float4 = *source;
        newVal.m_Float4.z = max(prevVal.m_Float4.z, value.z);
    } while (atomic_cmpxchg(((volatile __global int*)source) + 2, prevVal.m_4Ints[2], newVal.m_4Ints[2]) != prevVal.m_4Ints[2]);
    
    return prevVal.m_Float4;
}

__kernel void ComputeMinMax(  __global const float4* positions,
                            __local float4  *localMemory,
                            volatile __global float4 *globalMemory)
{
    size_t localId = get_local_id(0);
    size_t globalId = get_global_id(0);
    size_t localSize = get_local_size(0);
    size_t groupId = get_group_id(0);
    size_t groupSize = get_num_groups(0);


    // Find the the local min (the minimum for each work item)
    // First put the result in local memory...
    localMemory[localId] = positions[globalId];
    localMemory[localId + localSize] = positions[globalId];
    
    // Synchronize to make sure data is available for processing
    barrier(CLK_LOCAL_MEM_FENCE);
    
    // ...Then work in local
    int localStep = 1;
    size_t workingLocalSize = localSize;
    while (workingLocalSize > 1)
    {
        int indexInput = (localId / (2 * localStep)) * (2 * localStep);
        localMemory[indexInput] = min(localMemory[indexInput], localMemory[indexInput + localStep]);
        localMemory[indexInput + localSize] = max(localMemory[indexInput + localSize], localMemory[indexInput + localSize + localStep]);
        workingLocalSize /= 2;
        localStep *= 2;
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    AtomicMin(&globalMemory[0], localMemory[0]);
    AtomicMax(&globalMemory[1], localMemory[localSize]);
}


__kernel void CreateGrid(__global const float4* positions,
                        volatile __global float4 *globalMemory,
                        __global int2*   neighborsInfo,
                        __global int4*   gridInfo)
{
    size_t globalId = get_global_id(0);
    size_t groupSize = get_num_groups(0);

    float4 positionMin = globalMemory[0];
    float4 positionMax = globalMemory[1];
    
    float4 diff = positionMax - positionMin;
    int secondAxisLength = (int)diff.y;
    int thirdAxisLength  = (int)diff.z;

    gridInfo[0].y = secondAxisLength;
    gridInfo[0].z = thirdAxisLength;

    int xAxisProduct = secondAxisLength * thirdAxisLength;
    int yAxisProduct = thirdAxisLength;


    float4 currentPosition = positions[globalId];

    neighborsInfo[globalId].x = globalId;

    neighborsInfo[globalId].y = ((int)floor(currentPosition.x) - (int)floor(positionMin.x)) * xAxisProduct +
                                ((int)floor(currentPosition.y) - (int)floor(positionMin.y)) * yAxisProduct + 
                                ((int)floor(currentPosition.z) - (int)floor(positionMin.z));

}


// Local size should be bigger than 10 
__kernel void SortLocalGridCells(   __global int2* neighborsInfo,
                                    __global int2* neighborsInfo2)
{
    volatile __local int localBucket[10];
    volatile __local int localMaxCellIndex;
    
    size_t localId = get_local_id(0);
    size_t localSize = get_local_size(0);
    size_t globalId = get_global_id(0);
    size_t groupId = get_group_id(0);

    atomic_max(&localMaxCellIndex, neighborsInfo[globalId].y);

    int exp = 1;

    barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE);

    while (localMaxCellIndex / exp > 0)
    {
        if (localId < 10)
        {
            localBucket[localId] = 0;
        }

        barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE);
        
        int currentBucketIndex = (neighborsInfo[globalId].y / exp) % 10;

        atomic_inc(&localBucket[currentBucketIndex]);

        barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE);
        
        // Copy just once; only one work item will do it
        if (localId == 0)
        {
            for (int i = 1; i < 10; i++)
            {
                localBucket[i] += localBucket[i - 1];
            }
        }

        barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE);

       if (localId == 0)
       {
            for (int i = localSize - 1; i >= 0; i--)
            {
                int neighborIndex = i + (localSize * groupId);
                int bucketIndex = (neighborsInfo[neighborIndex].y / exp) % 10;
                int newIndex  = --localBucket[bucketIndex];
                newIndex += (localSize * groupId);
                neighborsInfo2[newIndex] = neighborsInfo[neighborIndex];
            }
        }

        barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE);
        
        // Copy the buffer in the data
        neighborsInfo[globalId] = neighborsInfo2[globalId];

        barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE);

        exp *= 10;
    }

    neighborsInfo2[globalId] = 0x7FFFFFFF;

    barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE);
}


__kernel void MergeSortGridCells(  __global int2* neighborsInfo,
                                   __global int2* neighborsInfo2,
                                   __local  int2* localMemory)
{
    size_t localId = get_local_id(0);
    size_t localSize = get_local_size(0);
    size_t globalId = get_global_id(0);
    size_t globalSize = get_global_size(0);
    size_t groupSize = get_num_groups(0);

    int2 value = neighborsInfo[globalId];

    int arrayPosition = 0;
    

    for(int i = 0; i < groupSize; i++)
    {
        // load one tile into local memory
        int idx = i * localSize + localId;
        localMemory[localId] = neighborsInfo[idx];

        // Synchronize to make sure data is available for processing
        barrier(CLK_LOCAL_MEM_FENCE);
        
        int minimum = 0;
        int maximum = localSize - 1;

        // calculate the midpoint for roughly equal partition
        int middle = (maximum - minimum) * 0.5f + minimum;

        bool wasBigger = true;
        bool found = false;

        // continue searching while [minimum,maximum] is not empty
        while (maximum >= minimum)
        {
            // determine which subarray to search
            if (value.y > localMemory[middle].y )
            {    
                // change min index to search upper subarray
                minimum = middle + 1;
                wasBigger = true;
            }
            else if (value.y < localMemory[middle].y)
            {
                // change max index to search lower subarray
                maximum = middle - 1;
                wasBigger = false;
            }
            else
            {
                found = true;
                // Start from the first cell with the same value
                while (middle - 1 >= minimum && value.y == localMemory[middle - 1].y)
                {
                    middle--;
                }
                int localImid = middle;
                // Sort by id to obtain an unique position in the output array even with the same cell index
                while (localImid <= maximum && value.y == localMemory[localImid].y)
                {
                    if (value.x > localMemory[localImid].x)
                    {
                        middle++;
                    }
                    localImid++;
                }
                break;
            }
            middle = (maximum - minimum) * 0.5f + minimum;
        }

        // The result can be different if we didn't found the samecell index
        if ( ! found)
        {
            // The difference is in function the last branch we have not found it, from the top or the bottom
            if ( (! wasBigger || middle >= maximum ) && maximum != -1)
            {
                middle++;
            }
        }
        

        arrayPosition += middle;
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    neighborsInfo2[arrayPosition] = value;
    
}

int GetNeighborsMax(__global const int2* neighborsInfo, 
                int currentIndex, int particlesCount, 
                int secondAxis, int thirdAxis)
{
    int nextNeighbors = currentIndex;
    const int currentPosition = neighborsInfo[currentIndex].y;
    const int sizePlane = thirdAxis * secondAxis;
    const int bottom = currentPosition + sizePlane;
    const int bottomBack = bottom + thirdAxis;
    const int bottomBackRight = bottomBack + 1;
    
    while ( nextNeighbors < particlesCount &&  
            bottomBackRight >= neighborsInfo[nextNeighbors].y)
    {
        nextNeighbors++;
    }

    return nextNeighbors - 1;
}

int GetNeighborsMin(__global const int2* neighborsInfo, 
                    int currentIndex, 
                int secondAxis, int thirdAxis)
{
    int previousNeighbors = currentIndex;
    const int currentPosition = neighborsInfo[currentIndex].y;
    const int sizePlane = thirdAxis * secondAxis;
    const int top = currentPosition - sizePlane;
    const int topBack = top - thirdAxis;
    const int topBackLeft = topBack - 1;

    while ( previousNeighbors >= 0 && 
            neighborsInfo[previousNeighbors].y >= topBackLeft)
    {
        previousNeighbors--;
    }

    return previousNeighbors + 1;
}

typedef struct 
{
    float4 m_Gravity;
    float m_GazConstant;
    float m_MuViscosity;
    float m_DeltaT;
    float m_Mass;
    float m_H;
} Parameters;

// positions has to be const because used in other work group
__kernel void ComputeSPH(__global const float4* positions, 
                 __global float4* previousPositions,
               __global const int2* neighborsInfo,
               __constant int4* particlesInfo,
               __constant Parameters* paramters,
               __global float4* newPositions,
               __global float* inputDensity,
               __global float* outputDensity)
{

    size_t globalId = get_global_id(0);
    int index = neighborsInfo[globalId].x;
    size_t globalSize = get_global_size(0);

    if (globalId >= particlesInfo[0].x)
        return;

    const float PI = 3.141592659f;
    float4 gravity = paramters[0].m_Gravity;
    const float mass = paramters[0].m_Mass;
    const float h = paramters[0].m_H;
    const float gazConstant = paramters[0].m_GazConstant;
    const float muViscosity = paramters[0].m_MuViscosity;
    const float sqrH = h * h;

    int particlesCount = particlesInfo[0].x;
    int secondAxis = particlesInfo[0].y;
    int thirdAxis = particlesInfo[0].z;

    const float deltaT = 1 / 60.0f;

    
    int minIndexNeighbor = GetNeighborsMin(neighborsInfo, globalId, secondAxis, thirdAxis);
    int maxIndexNeighbor = GetNeighborsMax(neighborsInfo, globalId, particlesCount, secondAxis, thirdAxis);


    float4 pressure = {0.0f, 0.0f, 0.0f, 0.0f};
    float4 viscosity = {0.0f, 0.0f, 0.0f, 0.0f};

    float4 currentPosition  = positions[index];
    float4 previsouPosition = previousPositions[index];
    float previousDensity = inputDensity[index];
    currentPosition.w = 0.0f;
    float density = 4.0f * mass  / (PI*sqrH*sqrH*sqrH) ;
    float positionData = positions[index].w;

    for (int j = minIndexNeighbor; j <= maxIndexNeighbor; j++)
    {
        int indexNeighbor =  neighborsInfo[j].x;

        float4 neighborPosition = positions[indexNeighbor];
        float4 neighborPrevisousPosition = previousPositions[indexNeighbor];

        float4 separation = currentPosition - neighborPosition;
        separation.w = 0.0f;
        float sqrDistance = dot(separation, separation);
            
        if (sqrDistance < sqrH)
        {
            float4 normal = separation;
            float distance = sqrt(sqrDistance);
            if (distance > 1e-5f/*0.0f*/ )
            {
                normal /= distance;
            }

            float previousDensityNeighbor = inputDensity[indexNeighbor];

             density += mass * ( 15.0f / (PI*h*sqrH) ) * ((1.0f - distance / h)*(1.0f - distance / h)*(1.0f - distance / h));
			// density += ((sqrH - sqrDistance )*(sqrH - sqrDistance )*(sqrH - sqrDistance ));
            pressure += mass / previousDensityNeighbor * 
                        ((gazConstant * previousDensity + gazConstant * previousDensityNeighbor) * 0.5f) * 
                        ( 45.0f / (PI*sqrH*sqrH) ) * ((1.0f - distance / h)*(1.0f - distance / h)*(1.0f - distance / h)) * normal;


            float4 veolocity = (currentPosition - previsouPosition) / deltaT;
            float4 neighborVeolocity = (neighborPosition - neighborPrevisousPosition) / deltaT;

            viscosity += (mass / previousDensityNeighbor) *  ((neighborVeolocity - veolocity) * 
                        ( 45.0f / (PI*sqrH*sqrH*sqrH) ) * (h - distance) );
                        
        }
    }

    pressure *= mass / previousDensity;
    viscosity *= muViscosity * mass / previousDensity;

    const float damping = 0.99f;
    const float dampingPlusOne = damping + 1.0f;
    

    float4 acceleration = pressure + viscosity + gravity;


    newPositions[index] = (currentPosition * dampingPlusOne - 
                                previousPositions[index] * damping) +
                                acceleration * deltaT * deltaT;

    

    previousPositions[index] = currentPosition;
    outputDensity[index] = density;

    newPositions[index].w = positionData;
}

__kernel void CopyBuffer(__global const float4* positions,  __global float4* newPositions)
{
    size_t globalId = get_global_id(0);
    newPositions[globalId] = positions[globalId];
}

__kernel void ComputeCollision( __global float4* positionsOutput,
                                __global float4* previousPositions,
                                __constant const float4* spheres,
                                __constant const float4* spheresIn,
                                __constant const float4* aabbs,
                                const int4 shapesCount)
{
	uint  globalIdCount = get_work_dim();
    size_t globalId = get_global_id(0);
    float4 newPosition = positionsOutput[globalId];
    float positionData = positionsOutput[globalId].w;

    previousPositions[globalId].w = 0.0f;

    const float friction = 0.5f;
    const float restitution = 1.0f;
    const float onePlusRestitution = 1.0f + restitution;

    for (int i = 0; i < shapesCount.x; i++)
    {
        newPosition.w = 0.0f;

        const float radius = spheres[i].w;
        const float4 difference = newPosition - spheres[i];
        difference.w = 0.0f;
        float sqrDistance = dot(difference, difference);

        if (sqrDistance < radius * radius)
        {
            // Comnpute new position
            float4 contactNormal = difference / sqrt(sqrDistance);
            newPosition = spheres[i] + contactNormal * radius;
            
            // Compute previous position to generate collision response considering restitution
            float4 previousPosition = previousPositions[globalId];
            float4 oppositeVelocity = previousPosition - newPosition;

            float projection = dot(contactNormal, oppositeVelocity);
            previousPosition -= (projection * onePlusRestitution) * contactNormal;

            previousPositions[globalId] = previousPosition - (previousPosition - newPosition)* (1.0f - friction);
        }
    }

    const int notImplementedSphereCount = 0;
    for (int i = 0; i < shapesCount.y; i++)
    {
        newPosition.w = 0.0f;

        const float radius = spheresIn[i].w;
        const float4 difference = newPosition - spheresIn[i];
        difference.w = 0.0f;
        float sqrDistance = dot(difference, difference);

        if (sqrDistance > radius * radius)
        {
            // Comnpute new position
            float4 contactNormal = difference / sqrt(sqrDistance);
            newPosition = spheresIn[i] + contactNormal * radius;
            
            // Compute previous position to generate collision response considering restitution
            float4 previousPosition = previousPositions[globalId];
            float4 oppositeVelocity = previousPosition - newPosition;

            float projection = dot(contactNormal, oppositeVelocity);
            previousPosition -= (projection * onePlusRestitution) * contactNormal;

            previousPositions[globalId] = previousPosition - (previousPosition - newPosition)* (1.0f - friction);
        }
    }

    const float bigMultiplier = 100000.0f;
	
	float noiseId =  globalId;
	noiseId = noiseId - 2*globalIdCount;
	const float4 noise = {-0.000000000456f * noiseId, 0.0000000005486f * noiseId, 0.0000000004864f * noiseId, 0.0f};

    for (int i = 0; i < shapesCount.z; i += 2)
    {
        float4 zero = {0, 0, 0, 0};
        float4 one  = {1, 1, 1, 1};

        // Comnpute contact normal
        int4 diff = convert_int4((newPosition - aabbs[i]) * bigMultiplier);
        float4 contactNormal = select(zero, one, diff);
        diff = convert_int4((aabbs[i + 1] - newPosition) * bigMultiplier);
        contactNormal -= select(zero, one, diff);

        // Comnpute new position
        float4 newPosition2 = clamp(newPosition, aabbs[i], aabbs[i + 1]);

        if (newPosition2.x != newPosition.x || newPosition2.y != newPosition.y || newPosition2.z != newPosition.z)
        {
            newPosition = newPosition2 + noise;
            // Compute previous position to generate collision response considering restitution
            float4 previousPosition = previousPositions[globalId];
            float4 oppositeVelocity = (previousPosition - newPosition);

            float projection = dot(contactNormal, oppositeVelocity);
            previousPosition -= (projection * onePlusRestitution) * contactNormal;

            previousPositions[globalId] = previousPosition - (previousPosition - newPosition)*friction;
        }

    }

    positionsOutput     [globalId]      = newPosition;
    positionsOutput     [globalId].w    = positionData;
    previousPositions   [globalId].w    = positionData;
}



typedef struct 
{
    int2 m_Indices;
    float m_RestDistance;
    int m_Pad;

} Spring;


__kernel void ComputeSpring(__global float4* positions,
                            __global const Spring* springs,
                            __local float4* localPositions)
{
    const int batchesCount = 12;
    size_t globalId = get_global_id(0);
    size_t globalIdCount = get_global_size(0);
    size_t localId = get_local_id(0);
    size_t localIdCount = get_local_size(0);
    size_t workGroupCount = get_num_groups(0);
    size_t workGroupId = get_group_id(0);


    size_t springId = batchesCount * globalId;
    size_t particlesCount = (2 * globalIdCount);    
    
    
    size_t particlesByWorkGroup = particlesCount / workGroupCount;
    size_t particlesByWorkItem = particlesByWorkGroup / localIdCount;

    size_t startGlobalParticlesForWorkGroup = workGroupId * particlesByWorkGroup;

    // Copy particles in local memory
    for (int i = 0; i < particlesByWorkItem; i++)
    {
        int idx = i * localIdCount + localId;
        localPositions[idx] = positions[startGlobalParticlesForWorkGroup + idx];
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    const float kStiffness = 0.8f;


    // Iterate through the dependant batches
    for (int i = 0; i < batchesCount; i ++)
    {
        int indexSpring = springId + i;
        int index1 = springs[indexSpring].m_Indices.x;
        int index2 = springs[indexSpring].m_Indices.y;
        float rest = springs[indexSpring].m_RestDistance;

        // Read local memory
        int index1InsideWorkGroup = (index1 % particlesByWorkGroup);
        int index2InsideWorkGroup = (index2 % particlesByWorkGroup);

        float4 position1 = localPositions[index1InsideWorkGroup];
        float4 position2 = localPositions[index2InsideWorkGroup];

        float4 separation = position2 - position1;
        separation.w = 0.0f;
        
        // float sqrDistance = dot(separation, separation);
        float distance = length(separation);

        // Handle particles without constraints in this batch
        if (rest == 0.0f)
        {
            rest = 1.0f;
            distance = 1.0f;
        }
    

        float4 movingVector = {0.0f, 0.0f, 0.0f, 0.0f};
        if (distance > 1e-2f)
        {
            movingVector = (rest / distance - 1.0f) * 0.5f * separation;
        }

        if (position1.w == 0.0f || position2.w == 0.0f)
        {
            movingVector *= 2.0f;
        }
        if (position1.w != 0.0f)
        {
            position1 -= kStiffness*movingVector;
        }
        if (position2.w != 0.0f)
        {
            position2 += kStiffness*movingVector;
        }

        barrier(CLK_LOCAL_MEM_FENCE);

        // Write in local memory
        localPositions[index1InsideWorkGroup] = position1;
        localPositions[index2InsideWorkGroup] = position2;
       
        barrier(CLK_LOCAL_MEM_FENCE);
       
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    // Write the output position
    for (int i = 0; i < particlesByWorkItem; i++)
    {
        int idx = i * localIdCount + localId;
        positions[idx + startGlobalParticlesForWorkGroup] = localPositions[idx];
    }
    
}


typedef struct
{
    float4  m_Position;
    float4  m_Direction;
    float   m_Radius;
    int     m_Type;
} Accelerator;


__kernel void ComputeAccelerator(__global float4* positions,
                                 __global float4* previousPositions,
                                 __global const Accelerator* accelerators,
                                 int acceleratorsCount)
{
    size_t globalId = get_global_id(0);


    float4 acceleration = {0.0f, 0.0f, 0.0f, 0.0f};

    float positionData = positions[globalId].w;


    const float sqrClosestActivity = 0.0f;
    bool hasKillSpeed = false;

    if (positionData != 0.0f)
    {   
        for (int i = 0; i < acceleratorsCount; i++)
        {

            switch (accelerators[i].m_Type)
            {

                case 0: // Force field + attraction
                {
                    float radius = accelerators[i].m_Radius;
                    float sqrRadius = radius * radius;
                    float4 position = positions[globalId];
                    float4 separation = accelerators[i].m_Position - position;
                    separation.w = 0.0f;
                    float sqrDistance = dot(separation, separation);

                    if (sqrDistance > sqrClosestActivity && sqrDistance < sqrRadius)
                    {
                        float distance = sqrt(sqrDistance);
                        float4 normal = separation / distance;

                        // Acceleration to the accelerator center
                        acceleration += normal * (radius / distance);

                        float4 perpendicular = cross(normal, accelerators[i].m_Direction);
                        perpendicular.w = 0.0f;

                        perpendicular = normalize(perpendicular);

                        // Circular acceleration 
                        acceleration += perpendicular * (radius / distance)*0.05f;
                    }
                    break;
                }
                case 1: // Simple homogenous force
                {
                    acceleration += accelerators[i].m_Direction;
                    break;
                }
                case 2: // Repulsion
                {
                    float radius = accelerators[i].m_Radius;
                    float sqrRadius = radius * radius;
                    float4 position = positions[globalId];
                    float4 separation = accelerators[i].m_Position - position;
                    separation.w = 0.0f;
                    float sqrDistance = dot(separation, separation);

                    if (sqrDistance > sqrClosestActivity && sqrDistance < sqrRadius)
                    {
                        float distance = sqrt(sqrDistance);
                        float4 normal = separation / distance;

                        // Acceleration from the accelerator center
                        acceleration -= normal * (radius / distance);
                    }
                    break;
                }
                case 3: // Attraction
                {
                    float radius = accelerators[i].m_Radius;
                    float sqrRadius = radius * radius;
                    float4 position = positions[globalId];
                    float4 separation = accelerators[i].m_Position - position;
                    separation.w = 0.0f;
                    float sqrDistance = dot(separation, separation);

                    if (sqrDistance > sqrClosestActivity && sqrDistance < sqrRadius)
                    {
                        float distance = sqrt(sqrDistance);
                        float4 normal = separation / distance;

                        // Acceleration from the accelerator center
                        acceleration += normal * (radius / distance);
                    }
                    break;
                }
                case 4: // Spiral
                {
                    float radius = accelerators[i].m_Radius;
                    float sqrRadius = radius * radius;
                    float4 position = positions[globalId];
                    float4 separation = accelerators[i].m_Position - position;
                    separation.w = 0.0f;
                    float sqrDistance = dot(separation, separation);

                    if (sqrDistance > sqrClosestActivity && sqrDistance < sqrRadius)
                    {
                        float distance = sqrt(sqrDistance);
                        float4 normal = separation / distance;

                        float4 perpendicular = cross(normal, accelerators[i].m_Direction);
                        perpendicular.w = 0.0f;

                        perpendicular = normalize(perpendicular);

                        // Circular acceleration 
                        acceleration += perpendicular * (radius / distance);
                    }
                    break;
                }
                case 5: // Kill speed
                {
                    hasKillSpeed = true;
                }
            }
        }
    }

    acceleration.w = 0.0f;

    const float deltaT = 1/60.0f;
    const float damping = 0.9999f;
    const float dampingPlusOne = 1.0f + damping;


    float4 newPosition = (positions[globalId] * dampingPlusOne - previousPositions[globalId] * damping) 
                                + acceleration * deltaT * deltaT;

    newPosition.w = positionData;
    
    // Store the previous position
    if ( ! hasKillSpeed)
    {
        previousPositions[globalId] = positions[globalId];
        positions   [globalId] = newPosition;
    }
    else
    {
        previousPositions[globalId] = positions[globalId];
    }

    
    
}

__kernel void ComputeAnimation( __global float4* positions,
                                __global const float4* startPositions,
                                __global const float4* endPositions,
                                const float animationTime)
{
    size_t globalId = get_global_id(0);

    float positionData = endPositions[globalId].w;
    positions[globalId] = startPositions[globalId] * (1.0f - animationTime) + endPositions[globalId] * animationTime;
    positions[globalId].w = positionData;
}
