#include "SpatialGrid.h"
#include <cmath>
#include <algorithm>

namespace NeoEngine {

SpatialGrid::SpatialGrid(float cellSize, float margin) {
    m_InvCellSize = 1.0f / cellSize;
    m_CellsPerAxis = static_cast<int>(WORLD_SIZE * m_InvCellSize) + 2;
    cellCount_ = m_CellsPerAxis * m_CellsPerAxis;
    cells_ = new Cell[cellCount_];
}

SpatialGrid::~SpatialGrid() {
    delete[] cells_;
}

void SpatialGrid::Insert(EntityID id, float x, float /*y*/, float z) {
    int ix = static_cast<int>(std::floor(x * m_InvCellSize));
    int iz = static_cast<int>(std::floor(z * m_InvCellSize));
    ix = std::clamp(ix, 0, m_CellsPerAxis - 1);
    iz = std::clamp(iz, 0, m_CellsPerAxis - 1);
    int idx = iz * m_CellsPerAxis + ix;
    Cell& cell = cells_[idx];
    if (cell.count < MAX_ENTITIES_PER_CELL) {
        cell.entities[cell.count++] = id;
    }
}

void SpatialGrid::Clear() {
    for (int i = 0; i < cellCount_; ++i) {
        cells_[i].count = 0;
    }
}

} // namespace
