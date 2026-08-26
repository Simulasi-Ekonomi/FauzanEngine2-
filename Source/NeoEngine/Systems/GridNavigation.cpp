#include "GridNavigation.h"

#include <algorithm>
#include <array>
#include <limits>

namespace NeoEngine {
bool GridNavigation::Fail(GridNavigationError error) const { lastError_ = error; return false; }
uint32_t GridNavigation::Index(GridCell cell) const { return static_cast<uint32_t>(cell.z) * side_ + cell.x; }
bool GridNavigation::Valid(GridCell cell) const { return side_ != 0 && cell.x < side_ && cell.z < side_; }
bool GridNavigation::Initialize(uint16_t side) {
    if (side < kMinSide || side > kMaxSide) return Fail(GridNavigationError::InvalidConfiguration);
    side_ = side; blocked_.assign(static_cast<size_t>(side) * side, 0); lastError_ = GridNavigationError::None; return true;
}
bool GridNavigation::SetBlocked(GridCell cell, bool blocked) {
    if (!Valid(cell)) return Fail(side_ == 0 ? GridNavigationError::NotInitialized : GridNavigationError::OutOfBounds);
    blocked_[Index(cell)] = blocked ? 1 : 0; lastError_ = GridNavigationError::None; return true;
}
bool GridNavigation::IsBlocked(GridCell cell) const { return !Valid(cell) || blocked_[Index(cell)] != 0; }
bool GridNavigation::FindPath(GridCell start, GridCell goal, std::vector<GridCell>& route) const {
    if (side_ == 0) return Fail(GridNavigationError::NotInitialized);
    if (!Valid(start) || !Valid(goal)) return Fail(GridNavigationError::OutOfBounds);
    if (IsBlocked(start) || IsBlocked(goal)) return Fail(GridNavigationError::BlockedEndpoint);
    const uint32_t count = static_cast<uint32_t>(side_) * side_;
    std::vector<int32_t> parent(count, -1);
    std::vector<uint32_t> queue; queue.reserve(count);
    const uint32_t begin = Index(start), end = Index(goal); parent[begin] = static_cast<int32_t>(begin); queue.push_back(begin);
    constexpr std::array<int32_t, 4> kDx{-1, 1, 0, 0}; constexpr std::array<int32_t, 4> kDz{0, 0, -1, 1};
    for (size_t head = 0; head < queue.size() && parent[end] == -1; ++head) {
        const uint32_t current = queue[head]; const GridCell cell{static_cast<uint16_t>(current % side_), static_cast<uint16_t>(current / side_)};
        for (size_t direction = 0; direction < kDx.size(); ++direction) {
            const int32_t nx = static_cast<int32_t>(cell.x) + kDx[direction], nz = static_cast<int32_t>(cell.z) + kDz[direction];
            if (nx < 0 || nz < 0 || nx >= side_ || nz >= side_) continue;
            const GridCell next{static_cast<uint16_t>(nx), static_cast<uint16_t>(nz)}; const uint32_t nextIndex = Index(next);
            if (blocked_[nextIndex] != 0 || parent[nextIndex] != -1) continue;
            parent[nextIndex] = static_cast<int32_t>(current); queue.push_back(nextIndex);
        }
    }
    if (parent[end] == -1) return Fail(GridNavigationError::Unreachable);
    std::vector<GridCell> reversed;
    for (uint32_t current = end;; current = static_cast<uint32_t>(parent[current])) {
        reversed.push_back({static_cast<uint16_t>(current % side_), static_cast<uint16_t>(current / side_)});
        if (current == begin) break;
        if (reversed.size() > kMaxRouteCells) return Fail(GridNavigationError::RouteCapacity);
    }
    std::vector<GridCell> candidate(reversed.rbegin(), reversed.rend()); route = std::move(candidate); lastError_ = GridNavigationError::None; return true;
}
} // namespace NeoEngine
