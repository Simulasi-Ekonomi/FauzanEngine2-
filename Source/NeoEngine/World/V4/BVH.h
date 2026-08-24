#pragma once
#include <vector>
#include <cstdint>
#include <algorithm>
#include <cmath>

namespace NeoEngine {

struct BoundingBox { float minX, minY, minZ, maxX, maxY, maxZ; };

union BoundingSphere { float centerX, centerY, centerZ, radius; };

struct BVHNode {
    BoundingBox box;
    std::vector<uint32_t> entities;
    BVHNode* left = nullptr;
    BVHNode* right = nullptr;
    bool isLeaf() const { return !left && !right; }
};

class BVH {
public:
    void Build(const std::vector<BoundingBox>& aabbs, const std::vector<uint32_t>& entityIDs) {
        Clear();
        m_Root = BuildRecursive(0, aabbs.size(), aabbs, entityIDs);
    }

    void Query(const BoundingBox& query, std::vector<uint32_t>& out) const {
        QueryRecursive(m_Root, query, out);
    }

    void Clear() { ClearRecursive(m_Root); m_Root = nullptr; }

private:
    BVHNode* m_Root = nullptr;

    BVHNode* BuildRecursive(int start, int end, const std::vector<BoundingBox>& aabbs,
                            const std::vector<uint32_t>& entityIDs) {
        if (start >= end) return nullptr;
        BVHNode* node = new BVHNode();
        // Leaf jika hanya satu entitas
        if (end - start == 1) {
            node->box = aabbs[start];
            node->entities.push_back(entityIDs[start]);
            return node;
        }
        // Pilih sumbu dengan extent terbesar
        BoundingBox merged = aabbs[start];
        for (int i = start+1; i < end; ++i) {
            merged.minX = std::min(merged.minX, aabbs[i].minX);
            merged.minY = std::min(merged.minY, aabbs[i].minY);
            merged.minZ = std::min(merged.minZ, aabbs[i].minZ);
            merged.maxX = std::max(merged.maxX, aabbs[i].maxX);
            merged.maxY = std::max(merged.maxY, aabbs[i].maxY);
            merged.maxZ = std::max(merged.maxZ, aabbs[i].maxZ);
        }
        node->box = merged;
        float ex = merged.maxX - merged.minX, ey = merged.maxY - merged.minY, ez = merged.maxZ - merged.minZ;
        int axis = (ex > ey) ? ((ex > ez) ? 0 : 2) : ((ey > ez) ? 1 : 2);
        // Sort berdasarkan axis
        std::sort(const_cast<std::vector<BoundingBox>&>(aabbs).begin() + start,
                  const_cast<std::vector<BoundingBox>&>(aabbs).begin() + end,
                  [axis](const BoundingBox& a, const BoundingBox& b) {
                      float ca = (axis==0 ? (a.minX+a.maxX)*0.5f : (axis==1 ? (a.minY+a.maxY)*0.5f : (a.minZ+a.maxZ)*0.5f));
                      float cb = (axis==0 ? (b.minX+b.maxX)*0.5f : (axis==1 ? (b.minY+b.maxY)*0.5f : (b.minZ+b.maxZ)*0.5f));
                      return ca < cb;
                  });
        int mid = (start + end) / 2;
        node->left = BuildRecursive(start, mid, aabbs, entityIDs);
        node->right = BuildRecursive(mid, end, aabbs, entityIDs);
        return node;
    }

    void QueryRecursive(const BVHNode* node, const BoundingBox& query, std::vector<uint32_t>& out) const {
        if (!node) return;
        // Intersection test AABB vs AABB
        if (node->box.minX <= query.maxX && node->box.maxX >= query.minX &&
            node->box.minY <= query.maxY && node->box.maxY >= query.minY &&
            node->box.minZ <= query.maxZ && node->box.maxZ >= query.minZ) {
            if (node->isLeaf()) {
                out.insert(out.end(), node->entities.begin(), node->entities.end());
            } else {
                QueryRecursive(node->left, query, out);
                QueryRecursive(node->right, query, out);
            }
        }
    }

    void ClearRecursive(BVHNode* node) {
        if (!node) return;
        ClearRecursive(node->left);
        ClearRecursive(node->right);
        delete node;
    }
};

} // namespace NeoEngine
