#pragma once

#include <cstdint>
#include <vector>

namespace NeoEngine {

struct GridCell {
    uint16_t x = 0;
    uint16_t z = 0;
    friend bool operator==(const GridCell&, const GridCell&) = default;
};

enum class GridNavigationError : uint8_t { None, InvalidConfiguration, NotInitialized, OutOfBounds, BlockedEndpoint, Unreachable, RouteCapacity };

class GridNavigation {
public:
    static constexpr uint16_t kMinSide = 4;
    static constexpr uint16_t kMaxSide = 128;
    static constexpr uint16_t kMaxRouteCells = 512;
    bool Initialize(uint16_t side);
    bool SetBlocked(GridCell cell, bool blocked);
    [[nodiscard]] bool IsBlocked(GridCell cell) const;
    bool FindPath(GridCell start, GridCell goal, std::vector<GridCell>& route) const;
    [[nodiscard]] uint16_t Side() const { return side_; }
    [[nodiscard]] GridNavigationError LastError() const { return lastError_; }
private:
    bool Valid(GridCell cell) const;
    bool Fail(GridNavigationError error) const;
    uint32_t Index(GridCell cell) const;
    uint16_t side_ = 0;
    std::vector<uint8_t> blocked_;
    mutable GridNavigationError lastError_ = GridNavigationError::None;
};

} // namespace NeoEngine
