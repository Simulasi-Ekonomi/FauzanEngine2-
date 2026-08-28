#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

struct Meshlet {
    uint32_t vertexOffset{};
    uint32_t vertexCount{};
    uint32_t triangleOffset{};
    uint32_t triangleCount{};
};

class MeshletRenderer {
public:
    static constexpr std::size_t MaxMeshlets = 4096;

    void AddMeshlet(const Meshlet& meshlet);

    bool TryAddMeshlet(const Meshlet& meshlet) {
        if (!Valid(meshlet) || meshlets.size() >= MaxMeshlets) return false;
        meshlets.push_back(meshlet);
        return true;
    }

    void Render();

    void Clear() { meshlets.clear(); }
    [[nodiscard]] std::size_t MeshletCount() const { return meshlets.size(); }
    [[nodiscard]] uint64_t TriangleCount() const {
        uint64_t total = 0;
        for (const auto& meshlet : meshlets) total += meshlet.triangleCount;
        return total;
    }
    [[nodiscard]] bool Validate() const {
        for (const auto& meshlet : meshlets) if (!Valid(meshlet)) return false;
        return true;
    }

private:
    static bool Valid(const Meshlet& meshlet) {
        return meshlet.vertexCount > 0U && meshlet.triangleCount > 0U &&
               meshlet.triangleCount <= meshlet.vertexCount * 2U;
    }
    std::vector<Meshlet> meshlets;
};
