#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <mutex>
namespace NeoEngine {
class FrameAllocator {
    std::vector<uint8_t> m_Buffer; size_t m_Offset=0; std::mutex m_Mutex; size_t m_FrameCount=0;
public:
    FrameAllocator(size_t sizeMB=64){m_Buffer.resize(sizeMB*1024*1024);}
    void* Allocate(size_t size,size_t alignment=16){
        std::lock_guard<std::mutex>lock(m_Mutex);
        size_t current=(size_t)(m_Buffer.data()+m_Offset); size_t aligned=(current+alignment-1)&~(alignment-1);
        size_t newOffset=aligned-(size_t)m_Buffer.data()+size;
        if(newOffset>m_Buffer.size())return nullptr;
        m_Offset=newOffset; m_FrameCount++; return (void*)aligned;
    }
    void Reset(){std::lock_guard<std::mutex>lock(m_Mutex);m_Offset=0;m_FrameCount=0;}
    size_t GetUsedBytes()const{return m_Offset;}
    size_t GetFrameCount()const{return m_FrameCount;}
};
}
