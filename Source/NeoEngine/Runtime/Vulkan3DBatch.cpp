#include "Vulkan3DBatch.h"

#include <limits>

namespace NeoEngine {

void Vulkan3DBatchBuilder::Reset() noexcept {
    commands_.clear();
    ranges_.clear();
    vertexCount_ = 0;
    indexCount_ = 0;
    instanceCount_ = 0;
}

bool Vulkan3DBatchBuilder::Append(uint32_t vertexCount, uint32_t indexCount, uint32_t instanceCount) noexcept {
    if (vertexCount == 0 || indexCount == 0 || instanceCount == 0) {
        return false;
    }

    const uint64_t nextVertex = static_cast<uint64_t>(vertexCount_) + vertexCount;
    const uint64_t nextIndex = static_cast<uint64_t>(indexCount_) + indexCount;
    const uint64_t nextInstance = static_cast<uint64_t>(instanceCount_) + instanceCount;
    if (nextVertex > std::numeric_limits<uint32_t>::max() ||
        nextIndex > std::numeric_limits<uint32_t>::max() ||
        nextInstance > std::numeric_limits<uint32_t>::max() ||
        vertexCount > static_cast<uint32_t>(std::numeric_limits<int32_t>::max()) ||
        vertexCount_ > static_cast<uint32_t>(std::numeric_limits<int32_t>::max())) {
        return false;
    }

    Vulkan3DBatchRange range{};
    range.vertexCount = vertexCount;
    range.indexCount = indexCount;
    range.instanceCount = instanceCount;
    range.firstVertex = vertexCount_;
    range.firstIndex = indexCount_;
    range.firstInstance = instanceCount_;

    Vulkan3DIndirectCommand command{};
    command.indexCount = indexCount;
    command.instanceCount = instanceCount;
    command.firstIndex = indexCount_;
    command.vertexOffset = static_cast<int32_t>(vertexCount_);
    command.firstInstance = instanceCount_;

    try {
        ranges_.push_back(range);
        commands_.push_back(command);
    } catch (...) {
        if (!ranges_.empty()) {
            ranges_.pop_back();
        }
        return false;
    }

    vertexCount_ = static_cast<uint32_t>(nextVertex);
    indexCount_ = static_cast<uint32_t>(nextIndex);
    instanceCount_ = static_cast<uint32_t>(nextInstance);
    return true;
}

std::span<const Vulkan3DIndirectCommand> Vulkan3DBatchBuilder::Commands() const noexcept {
    return commands_;
}

std::span<const Vulkan3DBatchRange> Vulkan3DBatchBuilder::Ranges() const noexcept {
    return ranges_;
}

} // namespace NeoEngine
