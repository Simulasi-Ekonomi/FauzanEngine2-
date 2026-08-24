#pragma once
#include "Octree.h"
#include "BVH.h"
#include "../SpatialGrid.h"  // Grid broad-phase yang sudah ada

namespace NeoEngine {

class HybridSpatial {
public:
    HybridSpatial(float worldSize, float cellSize)
        : m_Grid(worldSize, cellSize),
          m_Octree({-worldSize/2, -10, -worldSize/2, worldSize/2, 100, worldSize/2}, 6) {}

    // Masukkan entitas ke sistem spatial
    void Insert(uint32_t entityID, float x, float y, float z, bool isStatic) {
        m_Grid.Insert(entityID, x, y, z);
        if (isStatic) {
            m_Octree.Insert(entityID, x, y, z);
        } else {
            m_DynamicAABBs.push_back(BoundingBox{x-1,y-1,z-1,x+1,y+1,z+1});
            m_DynamicIDs.push_back(entityID);
            m_NeedsBVHRebuild = true;
        }
    }

    // Bangun ulang BVH jika perlu
    void UpdateBVH() {
        if (m_NeedsBVHRebuild && !m_DynamicAABBs.empty()) {
            m_BVH.Build(m_DynamicAABBs, m_DynamicIDs);
            m_NeedsBVHRebuild = false;
        }
    }

    // Query gabungan: grid broad-phase + octree (statis) + BVH (dinamis)
    std::vector<uint32_t> Query(float x, float y, float z, float radius) {
        UpdateBVH();
        std::vector<uint32_t> results;
        // Grid broad-phase
        m_Grid.QuerySphere(x, y, z, radius, results);
        // Octree (statis)
        // m_Octree.QueryRadius(x, y, z, radius, results);
        // BVH (dinamis)
        BoundingBox queryBox = {x-radius, y-radius, z-radius, x+radius, y+radius, z+radius};
        m_BVH.Query(queryBox, results);
        return results;
    }

private:
    SpatialGrid m_Grid;
    Octree m_Octree;
    BVH m_BVH;
    std::vector<BoundingBox> m_DynamicAABBs;
    std::vector<uint32_t> m_DynamicIDs;
    bool m_NeedsBVHRebuild = false;
};

} // namespace NeoEngine
