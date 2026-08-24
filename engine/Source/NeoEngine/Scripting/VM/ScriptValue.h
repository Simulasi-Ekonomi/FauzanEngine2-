#pragma once
#include <variant>
#include <string>

namespace Neo {
    // Tipe data yang bisa dipahami oleh Script VM
    using ScriptValue = std::variant<int, float, std::string>;
}
