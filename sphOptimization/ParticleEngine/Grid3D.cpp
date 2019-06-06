#include "Grid3D.h"
#include <cmath>
#include <windows.h>
#include "Utility/Timer.h"


Grid3D::Grid3D():   m_ParticlesAllocatedCount(0), 
                    m_ParticleCellOrder(NULL), 
                    m_ParticleCellOrderBuffer(NULL),
                    m_Grid(NULL)
{
}

 Grid3D::~Grid3D()
 {
    delete[] m_ParticleCellOrder;
    delete[] m_ParticleCellOrderBuffer;
    delete[] m_Grid;
 }

 Grid3D::ParticleCellOrder* Grid3D::GetParticleCellOrder() const
 {
    return m_ParticleCellOrder;
 }

 Grid3D::ParticleCellOrder* Grid3D::GetParticleCellOrderBuffer() const
 {
    return m_ParticleCellOrderBuffer;
 }

 int Grid3D::GetSecondAxisLength() const
 {
    return m_SecondAxisLength;
 }

 int Grid3D::GetThirdAxisLength() const
 {
    return m_ThirdAxisLength;
 }

 int *Grid3D::GetGridInfo()
 {
    return m_GridInfo;
 }


void Grid3D::Reallocate()
{
    if (m_ParticlesAllocatedCount < m_ParticlesCount)
    {
        m_ParticlesAllocatedCount = m_ParticlesCount;
        m_VirtualGridAllocatedCount = 50 * m_ParticlesCount;
        delete[] m_ParticleCellOrder;
        delete[] m_ParticleCellOrderBuffer;
        delete[] m_Grid;
        m_ParticleCellOrder         = new ParticleCellOrder[m_ParticlesAllocatedCount];
        m_ParticleCellOrderBuffer   = new ParticleCellOrder[m_ParticlesAllocatedCount];
        m_Grid                      = new int[m_VirtualGridAllocatedCount];

    }
}

void Grid3D::InitSizeGrid(slmath::vec4 *particlePositions, int particlesCount)
{
    m_ParticlesCount = particlesCount;
    Reallocate();

    m_MaxAABB = particlePositions[0];
    m_MinAABB = particlePositions[0];

    Timer::GetInstance()->StartTimerProfile();
    for (int i = 1; i < particlesCount; i++)
    {
        m_MaxAABB = slmath::max(particlePositions[i], m_MaxAABB);
        m_MinAABB = slmath::min(particlePositions[i], m_MinAABB);
    }

    Timer::GetInstance()->StopTimerProfile("Min on CPU");

    m_MaxAABB.x = std::floor(m_MaxAABB.x + 1.0f);
    m_MaxAABB.y = std::floor(m_MaxAABB.y + 1.0f);
    m_MaxAABB.z = std::floor(m_MaxAABB.z + 1.0f);

    m_MinAABB.x = std::floor(m_MinAABB.x );
    m_MinAABB.y = std::floor(m_MinAABB.y );
    m_MinAABB.z = std::floor(m_MinAABB.z );
}

void Grid3D::Initialize(slmath::vec4 *particlePositions, int particlesCount)
{
    Timer::GetInstance()->StartTimerProfile();
    
    InitSizeGrid(particlePositions, particlesCount);

    slmath::vec4 diff = m_MaxAABB - m_MinAABB;

    int xAxisProduct, yAxisProduct, zAxisProduct;

    // Sort on x
    if (diff.x > diff.y && diff.x > diff.z)
    {
        m_AxisOrder.m_FirstAxis = X_AXIS;
        m_FirstAxisLength = int (diff.x); 
        // then on y
        if (diff.y > diff.z)
        {
            m_AxisOrder.m_SecondAxis = Y_AXIS;
            m_SecondAxisLength = int (diff.y);
            m_ThirdAxisLength = int (diff.z);

            xAxisProduct = m_SecondAxisLength * m_ThirdAxisLength;
            yAxisProduct = m_ThirdAxisLength;
            zAxisProduct = 1;
        }
        // then on z
        else
        {
            m_AxisOrder.m_SecondAxis = Z_AXIS;
            m_SecondAxisLength = int (diff.z);
            m_ThirdAxisLength = int (diff.y);
            
            xAxisProduct = m_SecondAxisLength * m_ThirdAxisLength;
            zAxisProduct = m_ThirdAxisLength;
            yAxisProduct = 1;
        }
    }
    // Sort on y 
    else if (diff.y > diff.z)
    {
        m_AxisOrder.m_FirstAxis = Y_AXIS;
        m_FirstAxisLength = int (diff.y); 
        if (diff.x > diff.z)
        {
            m_AxisOrder.m_SecondAxis = X_AXIS;
            m_SecondAxisLength = int (diff.x);
            m_ThirdAxisLength = int (diff.z);
             
            yAxisProduct = m_SecondAxisLength * m_ThirdAxisLength;
            xAxisProduct = m_ThirdAxisLength;
            zAxisProduct = 1;
         }
         else
         {
            m_AxisOrder.m_SecondAxis = Z_AXIS;
            m_SecondAxisLength = int (diff.z);
            m_ThirdAxisLength = int (diff.x);
             
            yAxisProduct = m_SecondAxisLength * m_ThirdAxisLength;
            zAxisProduct = m_ThirdAxisLength;
            xAxisProduct = 1;
         }
    }
    // Sort on z
    else
    {
        m_AxisOrder.m_FirstAxis = Z_AXIS;
        m_FirstAxisLength = int (diff.z);
        if (diff.x > diff.y)
        {
            m_AxisOrder.m_SecondAxis = X_AXIS;
            m_SecondAxisLength = int (diff.x);
            m_ThirdAxisLength = int (diff.y);
            
            zAxisProduct = m_SecondAxisLength * m_ThirdAxisLength;
            xAxisProduct = m_ThirdAxisLength;
            yAxisProduct = 1;
        }
        else
        {
            m_AxisOrder.m_SecondAxis = Y_AXIS;
            m_SecondAxisLength = int (diff.y);
            m_ThirdAxisLength = int (diff.x);
            
            zAxisProduct = m_SecondAxisLength * m_ThirdAxisLength;
            yAxisProduct = m_ThirdAxisLength;
            xAxisProduct = 1;
         }
    }

    for (int i = 0; i < particlesCount; i++)
    {
        m_ParticleCellOrder[i].m_ParticleIndex = i;
        m_ParticleCellOrder[i].m_CellIndex =    int(std::floor (particlePositions[i].x) - std::floor(m_MinAABB.x)) * xAxisProduct +
                                                int(std::floor (particlePositions[i].y) - std::floor(m_MinAABB.y)) * yAxisProduct + 
                                                int(std::floor (particlePositions[i].z) - std::floor(m_MinAABB.z)) * zAxisProduct;
    }


    
    
    RadixSort();
    Timer::GetInstance()->StopTimerProfile("Create Grid");

    m_GridInfo[0] = m_ParticlesCount;
    m_GridInfo[1] = m_SecondAxisLength;
    m_GridInfo[2] = m_ThirdAxisLength;
    m_GridInfo[3] = 0;
}

void Grid3D::InitializeFullGrid(slmath::vec4 *particlePositions, int particlesCount)
{
    Timer::GetInstance()->StartTimerProfile();

    Initialize(particlePositions, particlesCount);

    m_ThirdVirtualAxisLength =  m_ThirdAxisLength; //std::max(int(sqrt(sqrt(float(m_VirtualGridAllocatedCount)))) , 2);
    assert(m_ThirdVirtualAxisLength > 0);

    m_SecondVirtualAxisLength = m_SecondAxisLength; // m_ThirdVirtualAxisLength;
    assert(m_SecondVirtualAxisLength > 0);

    m_FirstVirtualAxisLength = m_FirstAxisLength; // m_VirtualGridAllocatedCount / (m_SecondVirtualAxisLength * m_ThirdVirtualAxisLength);
    assert(m_FirstVirtualAxisLength > 0);


    while (m_FirstVirtualAxisLength * m_SecondVirtualAxisLength * m_ThirdVirtualAxisLength > m_VirtualGridAllocatedCount)
    {
        if (m_FirstVirtualAxisLength > m_SecondVirtualAxisLength && m_FirstVirtualAxisLength > m_ThirdVirtualAxisLength)
        {
            m_FirstVirtualAxisLength--;
        }
        else if (m_SecondVirtualAxisLength > m_ThirdVirtualAxisLength)
        {
            m_SecondVirtualAxisLength--;
        }
        else
        {
            m_ThirdVirtualAxisLength--;
        }
    }

    m_VirtualGridSize = m_FirstVirtualAxisLength * m_SecondVirtualAxisLength * m_ThirdVirtualAxisLength;
    assert(m_VirtualGridSize <=  m_VirtualGridAllocatedCount);

    int xAxisProduct, yAxisProduct, zAxisProduct;
    xAxisProduct = yAxisProduct = zAxisProduct = 1;

    int xAxisModulo, yAxisModulo, zAxisModulo;
    xAxisModulo = yAxisModulo = zAxisModulo = m_ThirdVirtualAxisLength;


    switch (m_AxisOrder.m_FirstAxis)
    {
        case X_AXIS:
        xAxisProduct = m_SecondVirtualAxisLength * m_ThirdVirtualAxisLength;
        xAxisModulo = m_FirstVirtualAxisLength;
        break;
        case Y_AXIS:
        yAxisProduct = m_SecondVirtualAxisLength * m_ThirdVirtualAxisLength;
        yAxisModulo = m_FirstVirtualAxisLength;
        break;
        case Z_AXIS:
        zAxisProduct = m_SecondVirtualAxisLength * m_ThirdVirtualAxisLength;
        zAxisModulo = m_FirstVirtualAxisLength;
        break;   
    }
    switch (m_AxisOrder.m_SecondAxis)
    {
        case X_AXIS:
        xAxisProduct = m_ThirdVirtualAxisLength;
        xAxisModulo = m_SecondVirtualAxisLength;
        break;
        case Y_AXIS:
        yAxisProduct = m_ThirdVirtualAxisLength;
        yAxisModulo = m_SecondVirtualAxisLength;
        break;
        case Z_AXIS:
        zAxisProduct = m_ThirdVirtualAxisLength;
        zAxisModulo = m_SecondVirtualAxisLength;
        break;
        
    }

    for (int i = 0; i < m_ParticlesCount; i++)
    {
        m_ParticleCellOrder[i].m_ParticleIndex = i;

        int xInGrid = (int(std::floor (particlePositions[i].x) - std::floor(m_MinAABB.x))) % xAxisModulo;
        int yInGrid = (int(std::floor (particlePositions[i].y) - std::floor(m_MinAABB.y))) % yAxisModulo;
        int zInGrid = (int(std::floor (particlePositions[i].z) - std::floor(m_MinAABB.z))) % zAxisModulo;

        m_ParticleCellOrder[i].m_CellIndex =  xInGrid * xAxisProduct +
                                              yInGrid * yAxisProduct + 
                                              zInGrid * zAxisProduct;

#ifdef DEBUG
        int cellIndex = m_ParticleCellOrder[i].m_CellIndex;
        int firtAxis = cellIndex / (m_SecondVirtualAxisLength * m_ThirdVirtualAxisLength);
        cellIndex -= firtAxis * (m_SecondVirtualAxisLength * m_ThirdVirtualAxisLength);
        int secondAxis = cellIndex / m_ThirdVirtualAxisLength;
        cellIndex -= secondAxis * m_ThirdVirtualAxisLength;
        int thirdAxis = cellIndex;

        if  (m_AxisOrder.m_FirstAxis == X_AXIS && m_AxisOrder.m_SecondAxis == Y_AXIS)
        {
            assert(xInGrid == firtAxis);
            assert(yInGrid == secondAxis);
            assert(zInGrid == thirdAxis);
        }
        else if (m_AxisOrder.m_FirstAxis == X_AXIS && m_AxisOrder.m_SecondAxis == Z_AXIS)
        {
            assert(xInGrid == firtAxis);
            assert(zInGrid == secondAxis);
            assert(yInGrid == thirdAxis);
        }
        else if (m_AxisOrder.m_FirstAxis == Y_AXIS && m_AxisOrder.m_SecondAxis == X_AXIS)
        {
            assert(yInGrid == firtAxis);
            assert(xInGrid == secondAxis);
            assert(zInGrid == thirdAxis);
        }
        else if (m_AxisOrder.m_FirstAxis == Y_AXIS && m_AxisOrder.m_SecondAxis == Z_AXIS)
        {
            assert(yInGrid == firtAxis);
            assert(zInGrid == secondAxis);
            assert(xInGrid == thirdAxis);
        }
        else if (m_AxisOrder.m_FirstAxis == Z_AXIS && m_AxisOrder.m_SecondAxis == Y_AXIS)
        {
            assert(zInGrid == firtAxis);
            assert(yInGrid == secondAxis);
            assert(xInGrid == thirdAxis);
        }
        else
        {
            assert(zInGrid == firtAxis);
            assert(xInGrid == secondAxis);
            assert(yInGrid == thirdAxis);
        }
#endif // DEBUG
        assert(m_ParticleCellOrder[i].m_CellIndex < m_VirtualGridSize);
    }

    for (int i = 0; i < m_VirtualGridSize; i++)
    {
        m_Grid[i] = -1;
    }

    RadixSort();

    for (int i = 0; i < m_ParticlesCount; i++)
    {
        int gridIndex = m_ParticleCellOrder[i].m_CellIndex;
        assert(gridIndex < m_VirtualGridSize);
        if (m_Grid[gridIndex] == -1)
        {
            m_Grid[gridIndex] = i;
        }
        else
        {
            m_Grid[gridIndex] = std::min(m_Grid[gridIndex], i);
        }
    }

    Timer::GetInstance()->StopTimerProfile("Create Grid");
}

void Grid3D::InitializeFixedGrid(slmath::vec4 *particlePositions, int particlesCount)
{
    const int gridSide = 128;
    const int gridSize = gridSide * gridSide * gridSide;

    m_MinAABB = particlePositions[0];

    for (int i = 1; i < particlesCount; i++)
    {
        m_MinAABB = slmath::min(particlePositions[i], m_MinAABB);
    }

    for (int i = 0; i < particlesCount; i++)
    {
        m_ParticleCellOrder[i].m_ParticleIndex = i;

        int xInGrid = ((int)floor(particlePositions[i].x) - (int)floor(m_MinAABB.x)) % gridSide;
        int yInGrid = ((int)floor(particlePositions[i].y) - (int)floor(m_MinAABB.y)) % gridSide;
        int zInGrid = ((int)floor(particlePositions[i].z) - (int)floor(m_MinAABB.z)) % gridSide;

         m_ParticleCellOrder[i].m_CellIndex =   xInGrid * (gridSide * gridSide) +
                                                yInGrid * gridSide + 
                                                zInGrid;

    }

    m_SecondAxisLength = gridSide;
    m_ThirdAxisLength  = gridSide;
    
    RadixSort();

    m_GridInfo[0] = m_ParticlesCount;
    m_GridInfo[1] = m_SecondAxisLength;
    m_GridInfo[2] = m_ThirdAxisLength;
    m_GridInfo[3] = 0;

    m_VirtualGridAllocatedCount = gridSize;

    delete []m_Grid;
    m_Grid = new int[m_VirtualGridAllocatedCount];

    // Clear grid
    for (int i = 0; i < m_VirtualGridAllocatedCount; i++)
    {
        m_Grid[i] = 0x7FffFFff;
    }

    // Initialize grid
    for (int i = 0; i < m_ParticlesCount; i++)
    {
        int cellIndex = m_ParticleCellOrder[i].m_CellIndex;

        assert(cellIndex < m_VirtualGridAllocatedCount);

        m_Grid[cellIndex] = slmath::min(m_Grid[cellIndex], i);
    }
}

int Grid3D::GetNeighborsFixedGrid(int currentIndex,  int *neighbors, int neighborsMaxCount) const
{
    int neighborsCount = 0;

    const int positiveIndexNeighbors [] = 
        {1, 127, 128, 129,
        128*128 - 128 - 1, 128*128 - 128, 128*128 - 128 + 1,
        128*128 - 1, 128*128, 128*128 + 1,
        128*128 + 128 - 1, 128*128 + 128, 128*128 + 128 + 1};

    int currentCellPosition =  m_ParticleCellOrder[currentIndex].m_CellIndex;

    

    for (int j = 0; j < 27; j++)
    {
        int sign = 1;
        if (j % 2 == 0)
        {
            sign = -1;
        }

        int relatifIndex = 0;
        if (j != 26)
        {
            int positiveIndexIndex = j / 2;
            relatifIndex = sign * positiveIndexNeighbors[positiveIndexIndex];
        }
        

        int indexRealGrid = currentCellPosition +  relatifIndex;

        if (indexRealGrid < 0 || indexRealGrid >= 128 * 128 * 128)
            continue;

        int cellIndex = m_Grid[indexRealGrid];

        
        if (cellIndex == 0x7FffFFff)
            continue;

        // int indexNeighbor = m_ParticleCellOrder[cellIndex].m_ParticleIndex;
        int cellIndexNeighbor = m_ParticleCellOrder[cellIndex].m_CellIndex;

        

        while (cellIndex < m_ParticlesCount && cellIndexNeighbor == m_ParticleCellOrder[cellIndex].m_CellIndex)
        {
            assert(neighborsCount <= neighborsMaxCount);
            neighbors[neighborsCount++] = cellIndex;
            
            cellIndex++;
        }
    }
    assert(neighborsCount != 0);
    return neighborsCount;
}

int Grid3D::GetNeighborsMax(int currentIndex)
{
    int nextNeighbors = currentIndex;
    const int currentPosition = m_ParticleCellOrder[currentIndex].m_CellIndex;
    const int sizePlane = m_ThirdAxisLength * m_SecondAxisLength;
    const int bottom = currentPosition + sizePlane;
    const int bottomBack = bottom + m_ThirdAxisLength;
    const int bottomBackRight = bottomBack + 1;
    
    while ( nextNeighbors < m_ParticlesCount &&  
            bottomBackRight >= m_ParticleCellOrder[nextNeighbors].m_CellIndex)
    {
        nextNeighbors++;
    }

    return nextNeighbors - 1;
}

int Grid3D::GetNeighborsMin(int currentIndex)
{
    int previousNeighbors = currentIndex;
    const int currentPosition = m_ParticleCellOrder[currentIndex].m_CellIndex;
    const int sizePlane = m_ThirdAxisLength * m_SecondAxisLength;
    const int top = currentPosition - sizePlane;
    const int topBack = top - m_ThirdAxisLength;
    const int topBackLeft = topBack - 1;

    while ( previousNeighbors >= 0 && 
            m_ParticleCellOrder[previousNeighbors].m_CellIndex >= topBackLeft)
    {
        previousNeighbors--;
    }

    return previousNeighbors + 1;
}

// Twenty seven cells to check !
int Grid3D::GetNeighborsByParticleOrder(int currentIndex,  int *neighbors, int neighborsMaxCount)
{
    UNREFERENCED_PARAMETER(neighborsMaxCount);
    assert(neighborsMaxCount > 0);
    // Itself is considered
    int neighborsCount = 0;
    neighbors[neighborsCount++] = currentIndex;

    // Compute direct neighbors on the principle axis; the longest one; three cells
    // First left...
    const int currentPosition = m_ParticleCellOrder[currentIndex].m_CellIndex;
    const int leftPosition = currentPosition - 1;
    const int currentLine = currentPosition / m_ThirdAxisLength;
    int previousNeighbors = currentIndex - 1;
    while (previousNeighbors >= 0 && 
    m_ParticleCellOrder[previousNeighbors].m_CellIndex / m_ThirdAxisLength == currentLine &&
    m_ParticleCellOrder[previousNeighbors].m_CellIndex >= leftPosition)
    {
        assert(neighborsCount < neighborsMaxCount);
        neighbors[neighborsCount++] = previousNeighbors--;
    }

    const int rightPosition = currentPosition + 1;
    int nextNeighbors = currentIndex+1;
    while (nextNeighbors < m_ParticlesCount &&
        m_ParticleCellOrder[nextNeighbors].m_CellIndex / m_ThirdAxisLength == currentLine &&
        rightPosition >= m_ParticleCellOrder[nextNeighbors].m_CellIndex)
    {
        assert(neighborsCount < neighborsMaxCount);
        neighbors[neighborsCount++] = nextNeighbors++;
    }

    // Find neighbor on the second main axis, 6 cells to checks
    // Process until front-left neighbor is found not excluded
    const int sizePlane = m_ThirdAxisLength * m_SecondAxisLength;
    const int currentPlane = currentPosition /  sizePlane;
    const int frontPosition = currentPosition - m_ThirdAxisLength;
    if (frontPosition >= 0)
    {
        const int frontRightPosition = frontPosition + 1;
        while (previousNeighbors >= 0 &&
            m_ParticleCellOrder[previousNeighbors].m_CellIndex > frontRightPosition)
        {
            previousNeighbors--;
        }
        // Three first cells on the plane; now add them until front-left
        const int frontLeftPosition = frontPosition - 1;
        const int frontLine = frontPosition /  m_ThirdAxisLength;
        UNUSED_PARAMETER(&frontLine);
        while (previousNeighbors >= 0 && 
                m_ParticleCellOrder[previousNeighbors].m_CellIndex / sizePlane == currentPlane &&
                m_ParticleCellOrder[previousNeighbors].m_CellIndex >= frontLeftPosition)
        {
            assert(neighborsCount < neighborsMaxCount);
            neighbors[neighborsCount++] = previousNeighbors;
            previousNeighbors--;
        }
    }
    // Process until back-left neighbor is found not excluded
    const int backPosition =  currentPosition + m_ThirdAxisLength;
    const int backLeftPosition = backPosition - 1;
    while (nextNeighbors < m_ParticlesCount &&  backLeftPosition > m_ParticleCellOrder[nextNeighbors].m_CellIndex)
    {
        nextNeighbors++;
    }
    // Three last cells on the plane
    // Process until back-right neighbor is found and add as neighbors
    const int backRightPosition = backPosition + 1;
    const int backLine = backPosition /  m_ThirdAxisLength;
    UNUSED_PARAMETER(&backLine);
    while (nextNeighbors < m_ParticlesCount && 
            m_ParticleCellOrder[nextNeighbors].m_CellIndex / sizePlane == currentPlane &&
            backRightPosition >= m_ParticleCellOrder[nextNeighbors].m_CellIndex)
    {
        assert(neighborsCount < neighborsMaxCount);
        neighbors[neighborsCount++] = nextNeighbors;
        nextNeighbors++;
    }
     
    // Find neighbor on the third main axis: 12 additional cells
    // The three rows on the top
    const int top = currentPosition - sizePlane;
    const int topPlane = top / sizePlane;
    UNUSED_PARAMETER(&topPlane);
    const int topFront = top + m_ThirdAxisLength;
    if (top >= 0)
    {
        const int topFrontRight = topFront  + 1;
        while (previousNeighbors >= 0 && m_ParticleCellOrder[previousNeighbors].m_CellIndex > topFrontRight)
        {
            previousNeighbors--;
        }
        const int topFrontLeft = topFront  - 1;
        const int topFrontLine = topFront / m_ThirdAxisLength;
        UNUSED_PARAMETER(&topFrontLine);
        while (previousNeighbors >= 0 && 
                m_ParticleCellOrder[previousNeighbors].m_CellIndex >= topFrontLeft)
        {
            assert(neighborsCount < neighborsMaxCount);
            neighbors[neighborsCount++] = previousNeighbors;
            previousNeighbors--;
        }
    }

    if (top >= 0)
    {
        const int topRight = top + 1;
        while (previousNeighbors >= 0 && m_ParticleCellOrder[previousNeighbors].m_CellIndex > topRight)
        {
            previousNeighbors--;
        }
        const int topLeft = top - 1;
        const int topLine = top / m_ThirdAxisLength;
        UNUSED_PARAMETER(&topLine);
        while (previousNeighbors >= 0 && 
                m_ParticleCellOrder[previousNeighbors].m_CellIndex >= topLeft)
        {
            assert(neighborsCount < neighborsMaxCount);
            neighbors[neighborsCount++] = previousNeighbors;
            previousNeighbors--;
        }
    }

    const int topBack = top - m_ThirdAxisLength;
    if (topBack >= 0)
    {
        const int topBackRight = topBack + 1;
        while (previousNeighbors >= 0 && m_ParticleCellOrder[previousNeighbors].m_CellIndex > topBackRight)
        {
            previousNeighbors--;
        }
        const int topBackLeft = topBack - 1;
        const int topBackLine = topBack / m_ThirdAxisLength;
        UNUSED_PARAMETER(&topBackLine);
        while (previousNeighbors >= 0 && 
                m_ParticleCellOrder[previousNeighbors].m_CellIndex >= topBackLeft)
        {
            assert(neighborsCount < neighborsMaxCount);
            neighbors[neighborsCount++] = previousNeighbors;
            previousNeighbors--;
        }
    }
    // The three rows on the bottom
    const int bottom = currentPosition + sizePlane;
    const int bottomFront = bottom - m_ThirdAxisLength;
    const int bottomFrontLeft = bottomFront - 1;
    while (nextNeighbors < m_ParticlesCount && bottomFrontLeft > m_ParticleCellOrder[nextNeighbors].m_CellIndex)
    {
        nextNeighbors++;
    }
    const int bottomFrontRight = bottomFront + 1;
    const int bottomPlane = bottom / sizePlane;
    UNUSED_PARAMETER(&bottomPlane);
    const int bottomFrontLine = bottomFront / m_ThirdAxisLength;
    UNUSED_PARAMETER(&bottomFrontLine);
    while (nextNeighbors < m_ParticlesCount &&  
        
            bottomFrontRight >= m_ParticleCellOrder[nextNeighbors].m_CellIndex)
    {
        assert(neighborsCount < neighborsMaxCount);
        neighbors[neighborsCount++] = nextNeighbors;
        nextNeighbors++;
    }

    const int bottomLeft = bottom  - 1;
    while (nextNeighbors < m_ParticlesCount && bottomLeft > m_ParticleCellOrder[nextNeighbors].m_CellIndex)
    {
        nextNeighbors++;
    }
    const int bottomRight = bottom  + 1;
    const int bottomLine = bottom / m_ThirdAxisLength;
        UNUSED_PARAMETER(&bottomLine);
    while (nextNeighbors < m_ParticlesCount &&  
            bottomRight >= m_ParticleCellOrder[nextNeighbors].m_CellIndex)
    {
        assert(neighborsCount < neighborsMaxCount);
        neighbors[neighborsCount++] = nextNeighbors;
        nextNeighbors++;
    }
        
    const int bottomBack = bottom + m_ThirdAxisLength;
    const int bottomBackLeft = bottomBack - 1;
    while (nextNeighbors < m_ParticlesCount && bottomBackLeft > m_ParticleCellOrder[nextNeighbors].m_CellIndex)
    {
        nextNeighbors++;
    }
    const int bottomBackRight = bottomBack + 1;
    const int bottomBackLine = bottomBack / m_ThirdAxisLength;
    UNUSED_PARAMETER(&bottomBackLine);
    while (nextNeighbors < m_ParticlesCount &&  
            bottomBackRight >= m_ParticleCellOrder[nextNeighbors].m_CellIndex)
    {
        assert(neighborsCount < neighborsMaxCount);
        neighbors[neighborsCount++] = nextNeighbors;
        nextNeighbors++;
    }

    return neighborsCount;
}

int Grid3D::GetNeighbors(int currentIndex,  int *neighbors, int neighborsMaxCount)
{
    Grid3D::ParticleCellOrder *particleOrder = GetParticleCellOrder();

    int indexOrder = 0;
    for (; indexOrder < m_ParticlesCount; indexOrder++)
    {
        if (particleOrder[indexOrder].m_ParticleIndex == currentIndex)
            break;
    }

    int neighborsCount = GetNeighborsByParticleOrderHeuristic(indexOrder, neighbors, neighborsMaxCount);
    assert(neighborsCount <= neighborsMaxCount);
    for (int j = 0; j < neighborsCount; j++)
    {
        int indexParticleNeighbor = particleOrder[neighbors[j]].m_ParticleIndex;
        neighbors[j] = indexParticleNeighbor;
    }
    
     return neighborsCount;
}

int Grid3D::GetNeighborsByParticleOrderFullGrid(int currentIndex,  int *neighbors, int neighborsMaxCount) const
{
    UNREFERENCED_PARAMETER(neighborsMaxCount);
    assert(neighborsMaxCount > 0);
    const int cellsCount = 27;
    int cellPositions[cellsCount];
    GetNeighborsCellIndexFullGrid2(currentIndex, cellPositions);

    int neighborsCount = 0;

    for (int cellIndex = 0; cellIndex < cellsCount; cellIndex++)
    {
        int cellPosition = cellPositions[cellIndex];

        if (cellPosition < 0)
        {
            cellPosition += m_VirtualGridSize;
        }

        cellPosition = cellPosition % m_VirtualGridSize;

        assert(cellPosition >= 0 && cellPosition < m_VirtualGridSize);


        int particleIndex = m_Grid[cellPosition];
        
        if (particleIndex == -1)
            continue;

        while (cellPosition == m_ParticleCellOrder[particleIndex].m_CellIndex)
        {
            assert(neighborsCount <= neighborsMaxCount);
            neighbors[neighborsCount++] = particleIndex++;
        }
    }

    assert(neighborsCount <= neighborsMaxCount);
    return neighborsCount;
}


int Grid3D::GetNeighborsByParticleOrderHeuristic(int currentIndex,  int *neighbors, int neighborsMaxCount)
{
    UNREFERENCED_PARAMETER(neighborsMaxCount);
    assert(neighborsMaxCount > 0);
    const int cellsCount = 27;
    int cellPositions[cellsCount];
    GetNeighborsPositionIndex(currentIndex, cellPositions);

    const int middleIndex = cellsCount / 2;

    int previousNeighbors = currentIndex;

    int neighborsCount = 0;
    int cellIndex = middleIndex;

    while (previousNeighbors >= 0 &&  
           cellPositions[0] <= m_ParticleCellOrder[previousNeighbors].m_CellIndex &&
           cellIndex >= 0 && cellPositions[cellIndex] >= 0)
        {
            const int cellPosition = cellPositions[cellIndex];
            const int particlePosition = m_ParticleCellOrder[previousNeighbors].m_CellIndex;

            if ( cellPosition == particlePosition)
            {
                neighbors[neighborsCount++] = previousNeighbors;
                previousNeighbors--;
            }
            else if (cellPosition > particlePosition)
            {
                cellIndex--;
            }
            else
            {
                
                if (cellIndex == 11 || cellIndex == 8 || cellIndex == 5 || cellIndex == 2)
                {
                    previousNeighbors = SearchDown(cellPosition, previousNeighbors);
                }
                else
                {
                    previousNeighbors--;
                }
            }
        }

    int nextNeighbors = currentIndex + 1;
    cellIndex = middleIndex;

    while (nextNeighbors < m_ParticlesCount &&  
           cellPositions[26] >= m_ParticleCellOrder[nextNeighbors].m_CellIndex &&
           cellIndex < 27)
        {
            const int cellPosition = cellPositions[cellIndex];
            const int particlePosition = m_ParticleCellOrder[nextNeighbors].m_CellIndex;

            if (cellPosition == particlePosition)
            {
                neighbors[neighborsCount++] = nextNeighbors;
                nextNeighbors++;
            }
            else if (cellPosition < particlePosition)
            {
                cellIndex++;
            }
            else
            {
                if (cellIndex == 15 || cellIndex == 18 || cellIndex == 21 || cellIndex == 24)
                {
                    nextNeighbors = SearchUp(cellPosition, nextNeighbors);
                }
                else
                {
                    nextNeighbors++;
                }
            }
        }

    assert(neighborsCount <= neighborsMaxCount);
    return neighborsCount;
}

int Grid3D::SearchDown(int positionToSearch, int currentPosition)
{
    int heuristic = currentPosition - (m_ParticleCellOrder[currentPosition].m_CellIndex - positionToSearch);
    heuristic = slmath::max(0, heuristic);

    if (positionToSearch >= m_ParticleCellOrder[heuristic].m_CellIndex)
    {
        while (positionToSearch >= m_ParticleCellOrder[heuristic+1].m_CellIndex)
        {
            heuristic++;
        }
    }
    else
    {
        while (heuristic >= 0 && positionToSearch < m_ParticleCellOrder[heuristic].m_CellIndex)
        {
            heuristic--;
        }
    }
    return heuristic;
}

int Grid3D::SearchUp(int positionToSearch, int currentPosition)
{
    int heuristic = currentPosition + (positionToSearch - m_ParticleCellOrder[currentPosition].m_CellIndex);
    heuristic = slmath::min(heuristic, m_ParticlesCount-1);

    if (positionToSearch <= m_ParticleCellOrder[heuristic].m_CellIndex)
    {
        while (positionToSearch <= m_ParticleCellOrder[heuristic-1].m_CellIndex)
        {
            heuristic--;
        }
    }
    else
    {
        while (heuristic < m_ParticlesCount && positionToSearch > m_ParticleCellOrder[heuristic].m_CellIndex)
        {
            heuristic++;
        }
    }
    return heuristic;
}

int Grid3D::ComputeHashNeighbors(int currentIndex,  int *neighbors, int neighborsMaxCount)const
{

    int cellsIndex[27];
    GetNeighborsPositionIndex(currentIndex, cellsIndex);

    int neighborsCount = 0;

    for (int i =  0; i < 27; i++)
    {       
        const int cellIndex = cellsIndex[i];

        if (cellIndex >= 0)
        {
            int index = SearchPosition(cellIndex);
            
            if (index < 0)
            {
                continue;
            }

            assert(index >= 0);

            while (index < m_ParticlesCount && m_ParticleCellOrder[index].m_CellIndex == cellIndex)
            {
                assert(neighborsCount < neighborsMaxCount);
                neighbors[neighborsCount++] = index++;
            }
        }
    }
    return neighborsCount;
}

int Grid3D::HashFunction(int cellToSearch) const
{
    float hashRatio = float(m_MaxCellUsed - m_MinCellUsed) / (m_ParticlesCount - 1);
    int indexToFind = int((cellToSearch - m_MinCellUsed) / hashRatio);

    return indexToFind;
}

int Grid3D::SearchPosition(int cellToSearch) const
{
    int indexToFind = HashFunction(cellToSearch);
    int indexToSearch = indexToFind;

    if (indexToFind < 0 || indexToFind >= m_ParticlesCount)
        return -1;

    while (indexToFind >= 0 && m_ParticleCellOrder[indexToFind].m_CellIndex != cellToSearch)
    {
        indexToFind--;
    }
    if (m_ParticleCellOrder[indexToFind].m_CellIndex != cellToSearch)
        indexToFind = indexToSearch;

    while (indexToFind < m_ParticlesCount && m_ParticleCellOrder[indexToFind].m_CellIndex != cellToSearch)
    {
        indexToFind++;
    }
    if (m_ParticleCellOrder[indexToFind].m_CellIndex != cellToSearch)
        return -1;

    return indexToFind;
}


void Grid3D::GetNeighborsCellIndexFullGrid2(int currentIndex, int positionsIndex[27]) const
{

    int cellIndex = m_ParticleCellOrder[currentIndex].m_CellIndex;


    int firtAxis = cellIndex / (m_SecondVirtualAxisLength * m_ThirdVirtualAxisLength);
    cellIndex -= firtAxis * (m_SecondVirtualAxisLength * m_ThirdVirtualAxisLength);
    int secondAxis = cellIndex / m_ThirdVirtualAxisLength;
    cellIndex -= secondAxis * m_ThirdVirtualAxisLength;
    int thirdAxis = cellIndex;

    int xInGrid, yInGrid, zInGrid;

    if  (m_AxisOrder.m_FirstAxis == X_AXIS && m_AxisOrder.m_SecondAxis == Y_AXIS)
    {
        xInGrid = firtAxis;
        yInGrid = secondAxis;
        zInGrid = thirdAxis;
    }
    else if (m_AxisOrder.m_FirstAxis == X_AXIS && m_AxisOrder.m_SecondAxis == Z_AXIS)
    {
        xInGrid = firtAxis;
        zInGrid = secondAxis;
        yInGrid = thirdAxis;
    }
    else if (m_AxisOrder.m_FirstAxis == Y_AXIS && m_AxisOrder.m_SecondAxis == X_AXIS)
    {
        yInGrid = firtAxis;
        xInGrid = secondAxis;
        zInGrid = thirdAxis;
    }
    else if (m_AxisOrder.m_FirstAxis == Y_AXIS && m_AxisOrder.m_SecondAxis == Z_AXIS)
    {
        yInGrid = firtAxis;
        zInGrid = secondAxis;
        xInGrid = thirdAxis;
    }
    else if (m_AxisOrder.m_FirstAxis == Z_AXIS && m_AxisOrder.m_SecondAxis == Y_AXIS)
    {
        zInGrid = firtAxis;
        yInGrid = secondAxis;
        xInGrid = thirdAxis;
    }
    else
    {
        zInGrid = firtAxis;
        xInGrid = secondAxis;
        yInGrid = thirdAxis;
    }


    int xAxisProduct, yAxisProduct, zAxisProduct;
    xAxisProduct = yAxisProduct = zAxisProduct = 1;

    int xAxisModulo, yAxisModulo, zAxisModulo;
    xAxisModulo = yAxisModulo = zAxisModulo = m_ThirdVirtualAxisLength;


    switch (m_AxisOrder.m_FirstAxis)
    {
        case X_AXIS:
        xAxisProduct = m_SecondVirtualAxisLength * m_ThirdVirtualAxisLength;
        xAxisModulo = m_FirstVirtualAxisLength;
        break;
        case Y_AXIS:
        yAxisProduct = m_SecondVirtualAxisLength * m_ThirdVirtualAxisLength;
        yAxisModulo = m_FirstVirtualAxisLength;
        break;
        case Z_AXIS:
        zAxisProduct = m_SecondVirtualAxisLength * m_ThirdVirtualAxisLength;
        zAxisModulo = m_FirstVirtualAxisLength;
        break;   
    }
    switch (m_AxisOrder.m_SecondAxis)
    {
        case X_AXIS:
        xAxisProduct = m_ThirdVirtualAxisLength;
        xAxisModulo = m_SecondVirtualAxisLength;
        break;
        case Y_AXIS:
        yAxisProduct = m_ThirdVirtualAxisLength;
        yAxisModulo = m_SecondVirtualAxisLength;
        break;
        case Z_AXIS:
        zAxisProduct = m_ThirdVirtualAxisLength;
        zAxisModulo = m_SecondVirtualAxisLength;
        break;
        
    }
    
    xInGrid += xAxisModulo;
    yInGrid += yAxisModulo;
    zInGrid += zAxisModulo;


    positionsIndex[0] =     (xInGrid) % xAxisModulo * xAxisProduct +
                            (yInGrid) % yAxisModulo * yAxisProduct +
                            (zInGrid) % zAxisModulo * zAxisProduct;

    positionsIndex[1] =     (xInGrid + 1) % xAxisModulo * xAxisProduct +
                            (yInGrid + 0) % yAxisModulo * yAxisProduct +
                            (zInGrid + 0) % zAxisModulo * zAxisProduct;

    positionsIndex[2] =     (xInGrid + 0) % xAxisModulo * xAxisProduct +
                            (yInGrid + 1) % yAxisModulo * yAxisProduct +
                            (zInGrid + 0) % zAxisModulo * zAxisProduct;

    positionsIndex[3] =     (xInGrid + 0) % xAxisModulo * xAxisProduct +
                            (yInGrid + 0) % yAxisModulo * yAxisProduct +
                            (zInGrid + 1) % zAxisModulo * zAxisProduct;

    positionsIndex[4] =     (xInGrid + 0) % xAxisModulo * xAxisProduct +
                            (yInGrid + 1) % yAxisModulo * yAxisProduct +
                            (zInGrid + 1) % zAxisModulo * zAxisProduct;

    positionsIndex[5] =     (xInGrid + 1) % xAxisModulo * xAxisProduct +
                            (yInGrid + 0) % yAxisModulo * yAxisProduct +
                            (zInGrid + 1) % zAxisModulo * zAxisProduct;

    positionsIndex[6] =     (xInGrid + 1) % xAxisModulo * xAxisProduct +
                            (yInGrid + 1) % yAxisModulo * yAxisProduct +
                            (zInGrid + 0) % zAxisModulo * zAxisProduct;

    positionsIndex[7] =     (xInGrid + 1) % xAxisModulo * xAxisProduct +
                            (yInGrid + 1) % yAxisModulo * yAxisProduct +
                            (zInGrid + 1) % zAxisModulo * zAxisProduct;

    positionsIndex[8] =     (xInGrid - 1) % xAxisModulo * xAxisProduct +
                            (yInGrid - 0) % yAxisModulo * yAxisProduct +
                            (zInGrid - 0) % zAxisModulo * zAxisProduct;

    positionsIndex[9] =     (xInGrid - 0) % xAxisModulo * xAxisProduct +
                            (yInGrid - 1) % yAxisModulo * yAxisProduct +
                            (zInGrid - 0) % zAxisModulo * zAxisProduct;

    positionsIndex[10] =    (xInGrid - 0) % xAxisModulo * xAxisProduct +
                            (yInGrid - 0) % yAxisModulo * yAxisProduct +
                            (zInGrid - 1) % zAxisModulo * zAxisProduct;

    positionsIndex[11] =    (xInGrid - 0) % xAxisModulo * xAxisProduct +
                            (yInGrid - 1) % yAxisModulo * yAxisProduct +
                            (zInGrid - 1) % zAxisModulo * zAxisProduct;

    positionsIndex[12] =    (xInGrid - 1) % xAxisModulo * xAxisProduct +
                            (yInGrid - 0) % yAxisModulo * yAxisProduct +
                            (zInGrid - 1) % zAxisModulo * zAxisProduct;

    positionsIndex[13] =    (xInGrid - 1) % xAxisModulo * xAxisProduct +
                            (yInGrid - 1) % yAxisModulo * yAxisProduct +
                            (zInGrid - 0) % zAxisModulo * zAxisProduct;

    positionsIndex[14] =    (xInGrid - 1) % xAxisModulo * xAxisProduct +
                            (yInGrid - 1) % yAxisModulo * yAxisProduct +
                            (zInGrid - 1) % zAxisModulo * zAxisProduct;

    positionsIndex[15] =    (xInGrid + 0) % xAxisModulo * xAxisProduct +
                            (yInGrid + 1) % yAxisModulo * yAxisProduct +
                            (zInGrid - 1) % zAxisModulo * zAxisProduct;

    positionsIndex[16] =    (xInGrid + 1) % xAxisModulo * xAxisProduct +
                            (yInGrid + 0) % yAxisModulo * yAxisProduct +
                            (zInGrid - 1) % zAxisModulo * zAxisProduct;

    positionsIndex[17] =    (xInGrid + 1) % xAxisModulo * xAxisProduct +
                            (yInGrid - 1) % yAxisModulo * yAxisProduct +
                            (zInGrid + 0) % zAxisModulo * zAxisProduct;

    positionsIndex[18] =    (xInGrid + 0) % xAxisModulo * xAxisProduct +
                            (yInGrid - 1) % yAxisModulo * yAxisProduct +
                            (zInGrid + 1) % zAxisModulo * zAxisProduct;

    positionsIndex[19] =    (xInGrid - 1) % xAxisModulo * xAxisProduct +
                            (yInGrid + 0) % yAxisModulo * yAxisProduct +
                            (zInGrid + 1) % zAxisModulo * zAxisProduct;

    positionsIndex[20] =    (xInGrid - 1) % xAxisModulo * xAxisProduct +
                            (yInGrid + 1) % yAxisModulo * yAxisProduct +
                            (zInGrid + 0) % zAxisModulo * zAxisProduct;

    positionsIndex[21] =    (xInGrid + 1) % xAxisModulo * xAxisProduct +
                            (yInGrid - 1) % yAxisModulo * yAxisProduct +
                            (zInGrid - 1) % zAxisModulo * zAxisProduct;

    positionsIndex[22] =    (xInGrid - 1) % xAxisModulo * xAxisProduct +
                            (yInGrid + 1) % yAxisModulo * yAxisProduct +
                            (zInGrid - 1) % zAxisModulo * zAxisProduct;

    positionsIndex[23] =    (xInGrid - 1) % xAxisModulo * xAxisProduct +
                            (yInGrid - 1) % yAxisModulo * yAxisProduct +
                            (zInGrid + 1) % zAxisModulo * zAxisProduct;

    positionsIndex[24] =    (xInGrid - 1) % xAxisModulo * xAxisProduct +
                            (yInGrid + 1) % yAxisModulo * yAxisProduct +
                            (zInGrid + 1) % zAxisModulo * zAxisProduct;

    positionsIndex[25] =    (xInGrid + 1) % xAxisModulo * xAxisProduct +
                            (yInGrid - 1) % yAxisModulo * yAxisProduct +
                            (zInGrid + 1) % zAxisModulo * zAxisProduct;

    positionsIndex[26] =    (xInGrid + 1) % xAxisModulo * xAxisProduct +
                            (yInGrid + 1) % yAxisModulo * yAxisProduct +
                            (zInGrid - 1) % zAxisModulo * zAxisProduct;



}

void Grid3D::GetNeighborsCellIndexFullGrid(int currentIndex, int positionsIndex[27]) const
{
    
    int currentPosition = m_ParticleCellOrder[currentIndex].m_CellIndex;
    const int middleIndex = 27 / 2;
    const int sizePlane = m_ThirdVirtualAxisLength * m_SecondVirtualAxisLength;

    // Direct neighbors
    positionsIndex[middleIndex] = currentPosition;
    int leftPosition = currentPosition - 1;
    leftPosition += sizePlane * (currentPosition / sizePlane - leftPosition / sizePlane);
    int previousNeighbors = middleIndex - 1;
    positionsIndex[previousNeighbors--] = leftPosition;
    const int rightPosition = currentPosition + 1;
    int nextNeighbors = middleIndex + 1;
    positionsIndex[nextNeighbors++] = rightPosition;

    // Front line
    int frontPosition = currentPosition - m_ThirdVirtualAxisLength;
    frontPosition += sizePlane * (currentPosition / sizePlane - frontPosition / sizePlane);
    const int frontRightPosition = frontPosition + 1;
    int frontLeftPosition = frontPosition - 1;
    frontLeftPosition += sizePlane * (frontPosition / sizePlane - frontLeftPosition / sizePlane);
    positionsIndex[previousNeighbors--] = frontRightPosition;
    positionsIndex[previousNeighbors--] = frontPosition;
    positionsIndex[previousNeighbors--] = frontLeftPosition;

    // Back line
    const int backPosition =  currentPosition + m_ThirdVirtualAxisLength;
    int backLeftPosition = backPosition - 1;
    backLeftPosition += sizePlane * (backPosition / sizePlane - backLeftPosition / sizePlane);
    const int backRightPosition = backPosition + 1;
    positionsIndex[nextNeighbors++] = backLeftPosition;
    positionsIndex[nextNeighbors++] = backPosition;
    positionsIndex[nextNeighbors++] = backRightPosition;

    // Top front line
    const int top = currentPosition - sizePlane;
    const int topFront = top + m_ThirdVirtualAxisLength;
    const int topFrontRight = topFront  + 1;
    int topFrontLeft = topFront  - 1;
    topFrontLeft += sizePlane * (topFront / sizePlane - topFrontLeft / sizePlane);
    positionsIndex[previousNeighbors--] = topFrontRight;
    positionsIndex[previousNeighbors--] = topFront;
    positionsIndex[previousNeighbors--] = topFrontLeft;

    // Top line
    const int topRight = top + 1;
    int topLeft = top - 1;
    topLeft += sizePlane * (top / sizePlane - topLeft / sizePlane);
    positionsIndex[previousNeighbors--] = topRight;
    positionsIndex[previousNeighbors--] = top;
    positionsIndex[previousNeighbors--] = topLeft;

    // Top back line
    int topBack = top - m_ThirdVirtualAxisLength;
    topBack += sizePlane * (top / sizePlane - topBack / sizePlane);
    const int topBackRight = topBack  + 1;
    int topBackLeft = topBack  - 1;
    topBackLeft += sizePlane * (topBack / sizePlane - topBackLeft / sizePlane);
    positionsIndex[previousNeighbors--] = topBackRight;
    positionsIndex[previousNeighbors--] = topBack;
    positionsIndex[previousNeighbors] = topBackLeft;
    assert(previousNeighbors == 0);

    // Bottom front line
    const int bottom = currentPosition + sizePlane;
    int bottomFront = bottom - m_ThirdVirtualAxisLength;
    bottomFront += sizePlane * (bottom / sizePlane - bottomFront / sizePlane);
    int bottomFrontLeft = bottomFront - 1;
    bottomFrontLeft += sizePlane * (bottomFront / sizePlane - bottomFrontLeft / sizePlane);
    const int bottomFrontRight = bottomFront + 1;
    positionsIndex[nextNeighbors++] = bottomFrontLeft;
    positionsIndex[nextNeighbors++] = bottomFront;
    positionsIndex[nextNeighbors++] = bottomFrontRight;

    // Bottom line
    int bottomLeft = bottom  - 1;
    bottomLeft += sizePlane * (bottom / sizePlane - bottomLeft / sizePlane);
    const int bottomRight = bottom  + 1;
    positionsIndex[nextNeighbors++] = bottomLeft;
    positionsIndex[nextNeighbors++] = bottom;
    positionsIndex[nextNeighbors++] = bottomRight;

    // Bottom back line
    const int bottomBack = bottom + m_ThirdVirtualAxisLength;
    int bottomBackLeft = bottomBack - 1;
    bottomBackLeft += sizePlane * (bottomBack / sizePlane - bottomBackLeft / sizePlane);
    const int bottomBackRight = bottomBack + 1;
    positionsIndex[nextNeighbors++] = bottomBackLeft;
    positionsIndex[nextNeighbors++] = bottomBack;
    positionsIndex[nextNeighbors] = bottomBackRight;

    assert(nextNeighbors == 26);

}

void Grid3D::GetNeighborsPositionIndex(int currentIndex, int positionsIndex[27]) const
{
    // Direct neighbors
    const int middleIndex = 27 / 2;
    int currentPosition = m_ParticleCellOrder[currentIndex].m_CellIndex;
    positionsIndex[middleIndex] = currentPosition;
    const int leftPosition = currentPosition - 1;
    int previousNeighbors = middleIndex - 1;
    positionsIndex[previousNeighbors--] = leftPosition;
    const int rightPosition = currentPosition + 1;
    int nextNeighbors = middleIndex + 1;
    positionsIndex[nextNeighbors++] = rightPosition;

    // Front line
    const int frontPosition = currentPosition - m_ThirdAxisLength;
    const int frontRightPosition = frontPosition + 1;
    const int frontLeftPosition = frontPosition - 1;
    positionsIndex[previousNeighbors--] = frontRightPosition;
    positionsIndex[previousNeighbors--] = frontPosition;
    positionsIndex[previousNeighbors--] = frontLeftPosition;

    // Back line
    const int backPosition =  currentPosition + m_ThirdAxisLength;
    const int backLeftPosition = backPosition - 1;
    const int backRightPosition = backPosition + 1;
    positionsIndex[nextNeighbors++] = backLeftPosition;
    positionsIndex[nextNeighbors++] = backPosition;
    positionsIndex[nextNeighbors++] = backRightPosition;

    // Top front line
    const int sizePlane = m_ThirdAxisLength * m_SecondAxisLength;
    const int top = currentPosition - sizePlane;
    const int topFront = top + m_ThirdAxisLength;
    const int topFrontRight = topFront  + 1;
    const int topFrontLeft = topFront  - 1;
    positionsIndex[previousNeighbors--] = topFrontRight;
    positionsIndex[previousNeighbors--] = topFront;
    positionsIndex[previousNeighbors--] = topFrontLeft;

    // Top line
    const int topRight = top + 1;
    const int topLeft = top - 1;
    positionsIndex[previousNeighbors--] = topRight;
    positionsIndex[previousNeighbors--] = top;
    positionsIndex[previousNeighbors--] = topLeft;

    // Top back line
    const int topBack = top - m_ThirdAxisLength;
    const int topBackRight = topBack  + 1;
    const int topBackLeft = topBack  - 1;
    positionsIndex[previousNeighbors--] = topBackRight;
    positionsIndex[previousNeighbors--] = topBack;
    positionsIndex[previousNeighbors] = topBackLeft;
    assert(previousNeighbors == 0);

    // Bottom front line
    const int bottom = currentPosition + sizePlane;
    const int bottomFront = bottom - m_ThirdAxisLength;
    const int bottomFrontLeft = bottomFront - 1;
    const int bottomFrontRight = bottomFront + 1;
    positionsIndex[nextNeighbors++] = bottomFrontLeft;
    positionsIndex[nextNeighbors++] = bottomFront;
    positionsIndex[nextNeighbors++] = bottomFrontRight;

    // Bottom line
    const int bottomLeft = bottom  - 1;
    const int bottomRight = bottom  + 1;
    positionsIndex[nextNeighbors++] = bottomLeft;
    positionsIndex[nextNeighbors++] = bottom;
    positionsIndex[nextNeighbors++] = bottomRight;

    // Bottom back line
    const int bottomBack = bottom + m_ThirdAxisLength;
    const int bottomBackLeft = bottomBack - 1;
    const int bottomBackRight = bottomBack + 1;
    positionsIndex[nextNeighbors++] = bottomBackLeft;
    positionsIndex[nextNeighbors++] = bottomBack;
    positionsIndex[nextNeighbors] = bottomBackRight;

    assert(nextNeighbors == 26);

}

int Grid3D::ConmputeCellsOnTrajectory(const slmath::vec3 &trajectory, const slmath::vec3 &position, 
                                            int *positionsIndex, int positionsMaxCount) const
{
    const float increment = 0.5f;
    UNREFERENCED_PARAMETER(positionsMaxCount);
    int positionsIndexCount = 0;
    
    float distance = slmath::length(trajectory);

    slmath::vec3 normal;
    if (distance == 0.0f)
    {
        normal = slmath::vec3(0.0f);
    }
    else
    {
        normal = trajectory / distance;
    }

    int positionIndex;

    slmath::vec3 particlePosition = position;

    while (distance >= 0.0f)
    {
        if (m_AxisOrder.m_FirstAxis == X_AXIS)
        {
            if (m_AxisOrder.m_SecondAxis == Y_AXIS)
            {
                positionIndex = int(std::floor (particlePosition.x) - std::floor(m_MinAABB.x)) 
                                        * (m_SecondAxisLength * m_ThirdAxisLength) +
                                int(std::floor (particlePosition.y) - std::floor(m_MinAABB.y)) 
                                        * m_ThirdAxisLength + 
                                int(std::floor (particlePosition.z) - std::floor(m_MinAABB.z));
            }
            else
            {
           
                positionIndex =  int(std::floor (particlePosition.x) - std::floor(m_MinAABB.x)) 
                                        * (m_SecondAxisLength * m_ThirdAxisLength) +
                                int(std::floor (particlePosition.z) - std::floor(m_MinAABB.z)) 
                                        * m_ThirdAxisLength +
                                int(std::floor (particlePosition.y) - std::floor(m_MinAABB.y));
            }
        }
        else if (m_AxisOrder.m_FirstAxis == Y_AXIS)
        {
                if (m_AxisOrder.m_SecondAxis == X_AXIS)
                {
                    positionIndex =    int(std::floor (particlePosition.y) - std::floor(m_MinAABB.y)) 
                                        * (m_SecondAxisLength * m_ThirdAxisLength) +
                                    int(std::floor (particlePosition.x) - std::floor(m_MinAABB.x)) 
                                        * m_ThirdAxisLength +
                                    int(std::floor (particlePosition.z) - std::floor(m_MinAABB.z));
                }
                else
                {
                    positionIndex =    int(std::floor (particlePosition.y) - std::floor(m_MinAABB.y)) 
                                        * (m_SecondAxisLength * m_ThirdAxisLength) +
                                    int(std::floor (particlePosition.z) - std::floor(m_MinAABB.z)) 
                                        * m_ThirdAxisLength +
                                    int(std::floor (particlePosition.x) - std::floor(m_MinAABB.x));
                }
        }
        else
        {
            if (m_AxisOrder.m_SecondAxis == X_AXIS)
            {
                positionIndex =    int(std::floor (particlePosition.z) - std::floor(m_MinAABB.z)) 
                                        * (m_SecondAxisLength * m_ThirdAxisLength) +
                                    int(std::floor (particlePosition.x) - std::floor(m_MinAABB.x)) 
                                        * m_ThirdAxisLength +
                                    int(std::floor (particlePosition.y) - std::floor(m_MinAABB.y));
            }
            else
            {
                positionIndex =    int(std::floor (particlePosition.z) - std::floor(m_MinAABB.z)) 
                                        * (m_SecondAxisLength * m_ThirdAxisLength) +
                                    int(std::floor (particlePosition.y) - std::floor(m_MinAABB.y)) 
                                        * m_ThirdAxisLength +
                                    int(std::floor (particlePosition.x) - std::floor(m_MinAABB.x));
            }
        }

        

        if ((positionsIndexCount == 0 || positionIndex != positionsIndex[positionsIndexCount - 1]) &&
              positionIndex >= 0)
        {
            assert(positionsIndexCount + 27 < positionsMaxCount);

            positionsIndex[positionsIndexCount++] = positionIndex;
            
            /*int index = 0;
            for (int i = 0; i < m_ParticlesCount; i++)
            {
                if (m_ParticleCellOrder[i].m_CellIndex == positionIndex)
                {
                    index = i;
                    break;
                }
            }
            GetNeighborsPositionIndex(index, &positionsIndex[positionsIndexCount]);
            positionsIndexCount += 27;

            for (int cellIndex = 0; cellIndex < positionsIndexCount; cellIndex++)
            {
                if (positionsIndex[cellIndex] < 0)
                {
                    positionsIndex[cellIndex] = positionsIndex[positionsIndexCount - 1];
                    positionsIndexCount--;
                }
                for (int cellIndex2 = cellIndex + 1; cellIndex2 < positionsIndexCount; cellIndex2++)
                {
                    if (positionsIndex[cellIndex] == positionsIndex[cellIndex2])
                    {
                        positionsIndex[cellIndex2] = positionsIndex[positionsIndexCount - 1];
                        positionsIndexCount--;
                    }
                }
            }*/
        }
        
         distance -= increment;
        particlePosition += normal *increment;

    }
    return positionsIndexCount;
}

int Grid3D::ComputeParticlesOnTrajectory(int currentIndex, const slmath::vec3 &trajectory, const slmath::vec3 &position, 
                                     int *positionsIndex, int positionsMaxCount) const
{
    UNREFERENCED_PARAMETER(positionsMaxCount);
    assert(positionsMaxCount > 27);
    const int maxCellsCount = 1024;
    int cellPositions[maxCellsCount];

     GetNeighborsPositionIndex(currentIndex, &cellPositions[0]);

    int cellsCount = ConmputeCellsOnTrajectory(trajectory, position, &cellPositions[27], maxCellsCount) + 27;
     assert(cellsCount <= maxCellsCount);

    int positionsIndexCount = 0;

    for (int i = 0; i < m_ParticlesCount; i++)
    {
        for (int cellIndex = 0; cellIndex < cellsCount; cellIndex++)
        {
            const int cellPosition = cellPositions[cellIndex];
            const int iPosition  = m_ParticleCellOrder[i].m_CellIndex;

            if (cellPosition == iPosition)
            {
                assert(positionsIndexCount <= positionsMaxCount);
                positionsIndex[positionsIndexCount++] = m_ParticleCellOrder[i].m_ParticleIndex;
            }
        }
    }
  
    assert(positionsIndexCount <= positionsMaxCount);
    return positionsIndexCount;
}


void Grid3D::RadixSort()
{
    int i;
    int m = m_ParticleCellOrder[0].m_CellIndex;
    int exp = 1;
    // Find the biggest cell index
    for (i = 0; i < m_ParticlesCount; i++)
    {
        if (m_ParticleCellOrder[i].m_CellIndex > m)
        m = m_ParticleCellOrder[i].m_CellIndex;
    }
 
    while (m / exp > 0)
    {
        int bucket[10] = { 0 };
        for (i = 0; i < m_ParticlesCount; i++)
        {
            bucket[(m_ParticleCellOrder[i].m_CellIndex / exp) % 10]++;
        }
        for (i = 1; i < 10; i++)
        {
            bucket[i] += bucket[i - 1];
        }
        for (i = m_ParticlesCount - 1; i >= 0; i--)
        {
            int bucketIndex = (m_ParticleCellOrder[i].m_CellIndex / exp) % 10;
            int newIndex  = --bucket[bucketIndex];
            m_ParticleCellOrderBuffer[newIndex] = m_ParticleCellOrder[i];
        }
        for (i = 0; i < m_ParticlesCount; i++)
        {
            m_ParticleCellOrder[i] = m_ParticleCellOrderBuffer[i];
        }
        exp *= 10;
    }
}

void Grid3D::HashCellIndex()
{
    for (int i = 0; i < m_ParticlesCount; i++)
    {
        // Empty the buffer
        m_ParticleCellOrderBuffer[i].m_CellIndex = -1;
    }

    // Init min and max, used to find the index, still in the order
    m_MinCellUsed = m_ParticleCellOrder[0].m_CellIndex;
    m_MaxCellUsed = m_ParticleCellOrder[m_ParticlesCount - 1].m_CellIndex;
    
    // Fill hash table buffer with data 
    for (int i = 0; i < m_ParticlesCount; i++)
    {
        int index = HashFunction(m_ParticleCellOrder[i].m_CellIndex);
        while (m_ParticleCellOrderBuffer[index].m_CellIndex != -1)
        {
            index++;
            if (index >= m_ParticlesCount)
            {
                index = 0;
            }
        }
        m_ParticleCellOrderBuffer[index] = m_ParticleCellOrder[i];
    }
    // Swap buffer
    ParticleCellOrder *buffer = m_ParticleCellOrderBuffer;
    m_ParticleCellOrderBuffer = m_ParticleCellOrder;
    m_ParticleCellOrder = buffer;

}

void Grid3D::CreateFullGrid()
{

    m_FirstVirtualAxisLength = m_ParticlesCount / (m_SecondAxisLength * m_ThirdAxisLength);
    assert(m_FirstVirtualAxisLength > 0);

    m_VirtualGridSize = m_FirstVirtualAxisLength * m_SecondAxisLength * m_ThirdAxisLength;
    assert(m_VirtualGridSize <=  m_ParticlesCount);

    for (int i = 0; i < m_ParticlesCount; i++)
    {
        m_Grid[i] = -1;
        m_ParticleCellOrder[i].m_CellIndex = m_ParticleCellOrder[i].m_CellIndex % m_VirtualGridSize;
    }

    RadixSort();

    for (int i = 0; i < m_ParticlesCount; i++)
    {
        int gridIndex = m_ParticleCellOrder[i].m_CellIndex;
        assert(gridIndex < m_ParticlesCount);
        if (m_Grid[gridIndex] == -1)
        {
            m_Grid[gridIndex] = i;
        }
        else
        {
            m_Grid[gridIndex] = std::min(m_Grid[gridIndex], i);
        }
    }
}
