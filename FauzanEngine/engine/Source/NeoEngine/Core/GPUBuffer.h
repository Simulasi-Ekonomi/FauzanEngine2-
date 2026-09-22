#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <atomic>
namespace NeoEngine{struct GPUBuffer{uint32_t id=0;size_t size=0;std::vector<uint8_t>data;static uint32_t NextId(){static std::atomic<uint32_t>n{1};return n.fetch_add(1,std::memory_order_relaxed);}GPUBuffer()=default;};}