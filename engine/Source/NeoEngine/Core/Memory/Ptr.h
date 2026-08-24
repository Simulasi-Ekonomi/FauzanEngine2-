#pragma once
#include <memory>

namespace Neo {
    template<typename T>
    using Unique = std::unique_ptr<T>;

    template<typename T, typename... Args>
    Unique<T> MakeUnique(Args&&... args) {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
}
