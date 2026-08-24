#pragma once
#include <vector>
#include <cstdint>
#include <algorithm>
#include <cmath>

namespace NeoEngine {

struct OctreeBounds { float minX, minY, minZ, maxX, maxY, maxZ; };

class OctreeNode {
public:
    OctreeBounds bounds;
    std::vector<uint32_t> entityIDs;
    OctreeNode* children[8] = {nullptr};
    bool isLeaf() const { return children[0] == nullptr; }
};

class Octree {
public:
    Octree(OctreeBounds worldBounds, int maxDepth = 6) : m_MaxDepth(maxDepth), m_Root(new OctreeNode()) {
        m_Root->bounds = worldBounds;
    }

    void Insert(uint32_t entityID, float x, float y, float z) {
        InsertRecursive(m_Root, entityID, x, y, z, 0);
    }

    void QueryRadius(float x, float y, float z, float radius, std::vector<uint32_t>& out) const {
        QueryRadiusRecursive(m_Root, x, y, z, radius, out);
    }

    void Clear() {
        ClearRecursive(m_Root);
        m_Root = new OctreeNode();
    }

private:
    int m_MaxDepth;
    OctreeNode* m_Root;

    void InsertRecursive(OctreeNode* node, uint32_t id, float x, float y, float z, int depth) {
        if (!node) return;
        if (depth >= m_MaxDepth || node->isLeaf()) {
            node->entityIDs.push_back(id);
            return;
        }
        // Tentukan child index
        float midX = (node->bounds.minX + node->bounds.maxX) * 0.5f;
        float midY = (node->bounds.minY + node->bounds.maxY) * 0.5f;
        float midZ = (node->bounds.minZ + node->bounds.maxZ) * 0.5f;
        int childIdx = (x >= midX) + (y >= midY)*2 + (z >= midZ)*4;
        if (!node->children[childIdx]) {
            node->children[childIdx] = new OctreeNode();
            node->children[childIdx]->bounds = {
                (x < midX) ? node->bounds.minX : midX, (y < midY) ? node->bounds.minY : midY, (z < midZ) ? node->bounds.minZ : midZ,
                (x < midX) ? midX : node->bounds.maxX, (y < midY) ? midY : node->bounds.maxY, (z < midZ) ? midZ : node->bounds.maxZ
            };
        }
        InsertRecursive(node->children[childIdx], id, x, y, z, depth+1);
    }

    void QueryRadiusRecursive(const OctreeNode* node, float x, float y, float z, float radius, std::vector<uint32_t>& out) const {
        if (!node) return;
        // Cek apakah node ini berpotongan dengan sphere
        float dx = std::max(node->bounds.minX - x, std::max(0.0f, x - node->bounds.maxX));
        float dy = std::max(node->bounds.minY - y, std::max(0.0f, y - node->bounds.maxY));
        float dz = std::max(node->bounds.minZ - z, std::max(0.0f, z - node->bounds.maxZ));
        if (dx*dx + dy*dy + dz*dz < radius*radius) {
            out.insert(out.end(), node->entityIDs.begin(), node->entityIDs.end());
            for (int i = 0; i < 8; ++i) QueryRadiusRecursive(node->children[i], x, y, z, radius, out);
        }
    }

    void ClearRecursive(OctreeNode* node) {
        if (!node) return;
        for (int i = 0; i < 8; ++i) { ClearRecursive(node->children[i]); delete node->children[i]; }
        delete node;
    }
};

} // namespace NeoEngine
