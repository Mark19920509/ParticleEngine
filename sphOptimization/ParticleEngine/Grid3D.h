
#ifndef GRID_3D
#define GRID_3D

#include <slmath/slmath.h>

class Grid3D
{

public:
    Grid3D();
    ~Grid3D();
    void Initialize(slmath::vec4 *particlePositions, int particlesCount);
    void InitializeFullGrid(slmath::vec4 *particlePositions, int particlesCount);
    void InitializeFixedGrid(slmath::vec4 *particlePositions, int particlesCount);

    // Returns neighbors by particles positions index
    int GetNeighbors(int currentIndex,  int *neighbors, int neighborsMaxCount);

    // Returns neighbors with the index in ParticleCellOrder array
    int GetNeighborsByParticleOrder(int currentIndex,  int *neighbors, int neighborsMaxCount);
    int GetNeighborsByParticleOrderHeuristic(int currentIndex,  int *neighbors, int neighborsMaxCount);
    int ComputeHashNeighbors(int currentIndex,  int *neighbors, int neighborsMaxCount)const;
    int GetNeighborsByParticleOrderFullGrid(int currentIndex,  int *neighbors, int neighborsMaxCount)const;
    int GetNeighborsFixedGrid(int currentIndex,  int *neighbors, int neighborsMaxCount) const;

    int GetNeighborsMax(int currentIndex);
    int GetNeighborsMin(int currentIndex);
    
    int GetSecondAxisLength() const;
    int GetThirdAxisLength() const;
    int *GetGridInfo();

    void GetNeighborsPositionIndex(int currentIndex, int positionsIndex[27]) const;
    void GetNeighborsCellIndexFullGrid(int currentIndex, int positionsIndex[27]) const;
    void GetNeighborsCellIndexFullGrid2(int currentIndex, int positionsIndex[27]) const;
    

    struct ParticleCellOrder
    {
        int m_ParticleIndex;
        int m_CellIndex;
    };

    ParticleCellOrder *GetParticleCellOrder()const;
    ParticleCellOrder *GetParticleCellOrderBuffer()const;


    int ComputeParticlesOnTrajectory(  int currentIndex,
                                        const slmath::vec3 &trajectory, 
                                        const slmath::vec3 &position,
                                        int *positionsIndex, 
                                        int  positionsMaxCount) const;

private:

    void InitSizeGrid(slmath::vec4 *particlePositions, int particlesCount);
    
    int ConmputeCellsOnTrajectory(  const slmath::vec3 &trajectory, 
                                    const slmath::vec3 &position,
                                    int *positionsIndex, 
                                    int  positionsMaxCount) const;

    int SearchDown(int positionToSearch, int currentPosition);
    int SearchUp(int positionToSearch, int currentPosition);
    int SearchPosition(int positionToSearch) const;
    
    int HashFunction(int cellToSearch)const;
    void RadixSort();
    void HashCellIndex();

    void CreateFullGrid();
    void Reallocate();

    slmath::vec4 m_MaxAABB;
    slmath::vec4 m_MinAABB;
    int m_FirstAxisLength;
    int m_SecondAxisLength;
    int m_ThirdAxisLength;

    int m_VirtualGridAllocatedCount;
    int m_VirtualGridSize;
    int m_FirstVirtualAxisLength;
    int m_SecondVirtualAxisLength;
    int m_ThirdVirtualAxisLength;

    ParticleCellOrder *m_ParticleCellOrder;
    ParticleCellOrder *m_ParticleCellOrderBuffer;
    int               *m_Grid;

    int m_ParticlesAllocatedCount;
    int m_ParticlesCount;

    int m_MaxCellUsed;
    int m_MinCellUsed;
    
    struct
    {
        unsigned int m_FirstAxis  : 2;
        unsigned int m_SecondAxis : 2;
    } m_AxisOrder;

    enum 
    {
        X_AXIS = 1, Y_AXIS = 2, Z_AXIS = 3
    };


    int m_GridInfo[4];

};


#endif //GRID_3D