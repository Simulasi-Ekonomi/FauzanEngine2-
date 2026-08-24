#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <cstdlib>
namespace NeoEngine {
class PoolAllocator {
    struct FreeNode{FreeNode* next;};
    void* m_Memory=nullptr; FreeNode* m_FreeList=nullptr; size_t m_BlockSize,m_BlockCount;
public:
    PoolAllocator(size_t blockSize,size_t blockCount):m_BlockSize(blockSize),m_BlockCount(blockCount){
        size_t total=m_BlockSize*m_BlockCount; m_Memory=malloc(total);
        for(size_t i=0;i<m_BlockCount;i++){FreeNode* node=(FreeNode*)((uint8_t*)m_Memory+i*m_BlockSize);node->next=m_FreeList;m_FreeList=node;}
    }
    ~PoolAllocator(){free(m_Memory);}
    void* Allocate(){if(!m_FreeList)return nullptr;FreeNode* node=m_FreeList;m_FreeList=node->next;return node;}
    void Free(void* ptr){FreeNode* node=(FreeNode*)ptr;node->next=m_FreeList;m_FreeList=node;}
    size_t GetBlockSize()const{return m_BlockSize;}
};
}
