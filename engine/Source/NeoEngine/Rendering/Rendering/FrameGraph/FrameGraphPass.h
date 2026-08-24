#pragma once
#include <functional>
#include <vector>
#include <string>

namespace Neo {
    class FrameGraphPass {
    public:
        std::string name;
        [[maybe_unused]] std::vector<std::string> reads;  // Resource yang dibutuhkan (Input)
        [[maybe_unused]] std::vector<std::string> writes; // Resource yang dihasilkan (Output)
        std::function<void()> execute;

        FrameGraphPass(const std::string& n, std::function<void()> fn)
            : name(n), execute(fn) {}

        void addInput(const std::string& resource) { reads.push_back(resource); }
        void addOutput(const std::string& resource) { writes.push_back(resource); }
    };
}
