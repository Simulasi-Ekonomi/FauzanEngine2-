#pragma once
#include <variant>
#include <string>

namespace NeoEngine {
    // Tipe data yang bisa dipahami oleh Script VM
    using ScriptValue = std::variant<int, float, std::string>;
}
