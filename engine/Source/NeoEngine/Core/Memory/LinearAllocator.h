#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>
namespace NeoEngine {
class LinearAllocator {
    void* m_Memory=nullptr; size_t m_Capacity=0,m_Offset=0; bool m_Owner=false;
public:
    LinearAllocator(size_t size){m_Memory=malloc(size);m_Capacity=size;m_Owner=true;}
    LinearAllocator(void* ptr,size_t size):m_Memory(ptr),m_Capacity(size),m_Owner(false){}
    ~LinearAllocator(){if(m_Owner&&m_Memory)free(m_Memory);}
    void* Allocate(size_t size,size_t alignment=8){size_t current=(size_t)((uint8_t*)m_Memory+m_Offset);size_t aligned=(current+alignment-1)&~(alignment-1);size_t newOffset=aligned-(size_t)m_Memory+size;if(newOffset>m_Capacity)return nullptr;m_Offset=newOffset;return (void*)aligned;}
    void Reset(){m_Offset=0;}
    size_t GetUsed()const{return m_Offset;}
    size_t GetCapacity()const{return m_Capacity;}
};
}
