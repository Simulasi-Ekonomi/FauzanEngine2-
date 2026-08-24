#pragma once
#include <cstdint>
namespace NeoEngine {
using EntityID = uint32_t;
class SpatialGrid {
public:
    static constexpr float WORLD_SIZE = 200.0f;
    SpatialGrid(float cellSize, float margin = 0.5f) {
        m_InvCellSize = 1.0f / cellSize;
        m_CellsPerAxis = static_cast<int>(WORLD_SIZE * m_InvCellSize) + 1;
        if (m_CellsPerAxis < 2) m_CellsPerAxis = 2;
        cellCount_ = m_CellsPerAxis * m_CellsPerAxis;
        cells_ = new Cell[cellCount_];
        Clear();
    }
    ~SpatialGrid() { delete[] cells_; }
    void Insert(EntityID id, float x, float y, float z) {
        int ix = static_cast<int>(x * m_InvCellSize), iz = static_cast<int>(z * m_InvCellSize);
        if (ix < 0) ix = 0; else if (ix >= m_CellsPerAxis) ix = m_CellsPerAxis - 1;
        if (iz < 0) iz = 0; else if (iz >= m_CellsPerAxis) iz = m_CellsPerAxis - 1;
        int cellIdx = iz * m_CellsPerAxis + ix;
        Cell& cell = cells_[cellIdx];
        if (cell.count < MAX_ENTITIES_PER_CELL) cell.entities[cell.count++] = id;
    }
    void Clear() { for (int i = 0; i < cellCount_; ++i) cells_[i].count = 0; }
    int GetCellCount() const { return cellCount_; }
    template<typename Func> void ForEachNonEmptyCell(Func&& func) {
        for (int i = 0; i < cellCount_; ++i) { const Cell& c = cells_[i]; if (c.count > 0) func(i, c.entities, c.count); }
    }
    template<typename Func> void ForEachPairInCellAndNeighbors(int cellIdx, Func&& func) {
        int ci = cellIdx % m_CellsPerAxis, cj = cellIdx / m_CellsPerAxis;
        const Cell& cell = cells_[cellIdx];
        for (int i = 0; i < cell.count; ++i) for (int j = i + 1; j < cell.count; ++j) func(cell.entities[i], cell.entities[j]);
        if (ci + 1 < m_CellsPerAxis) checkCellPair(cell, cells_[cj * m_CellsPerAxis + (ci + 1)], func);
        if (cj + 1 < m_CellsPerAxis) checkCellPair(cell, cells_[(cj + 1) * m_CellsPerAxis + ci], func);
        if (ci + 1 < m_CellsPerAxis && cj + 1 < m_CellsPerAxis) checkCellPair(cell, cells_[(cj + 1) * m_CellsPerAxis + (ci + 1)], func);
        if (ci - 1 >= 0 && cj + 1 < m_CellsPerAxis) checkCellPair(cell, cells_[(cj + 1) * m_CellsPerAxis + (ci - 1)], func);
    }
private:
    static constexpr int MAX_ENTITIES_PER_CELL = 512;
    struct Cell { EntityID entities[MAX_ENTITIES_PER_CELL]; int count = 0; };
    template<typename Func> static void checkCellPair(const Cell& a, const Cell& b, Func&& func) {
        for (int i = 0; i < a.count; ++i) for (int j = 0; j < b.count; ++j) func(a.entities[i], b.entities[j]);
    }
    Cell* cells_ = nullptr;
    int cellCount_ = 0;
    float m_InvCellSize = 1.0f;
    int m_CellsPerAxis = 1;
};
} // namespace
