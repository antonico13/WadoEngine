#include <Windows.h>

#include "WdMalloc.h"

#include <iostream>
#include <stdexcept>

#include <intrin.h>

namespace Wado::Malloc {

        // Sys info stuff 
        size_t WdMalloc::cacheLineSize;
        // Objects > block size -> large objects
        // Objects <= block size but > mediumBoundary -> medium
        // Objects <= mediumBoundary => medium 
        size_t WdMalloc::blockSize;
        size_t WdMalloc::mediumBoundary;
        size_t WdMalloc::coreCount;

        size_t WdMalloc::areaSize;
        // reserved Area is also the start of the higher level page map area 
        void *WdMalloc::reservedArea;

            // page map area is split up into pages 
        void *WdMalloc::pageMap;
        void *WdMalloc::pageMapArea;
        size_t WdMalloc::pageSize; 

        // start of allocation area 
        void *WdMalloc::allocationArea;
        // Bump pointer for current allocation (everything block based)
        size_t WdMalloc::currentAllocBumpPtr;

        void *WdMalloc::allocatorArea;
        size_t WdMalloc::sizeClassSizes[255];    

    void WdMalloc::mallocInit(const size_t initialSize, const size_t coreCount) {
        LPSYSTEM_INFO sysInfo = (LPSYSTEM_INFO) HeapAlloc(GetProcessHeap(), 0, sizeof(SYSTEM_INFO));

        if (sysInfo == NULL) {
            throw std::runtime_error("Could not get system info");
        };

        GetSystemInfo(sysInfo);

        // Need to get the cache line size 
        
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX systemLogicalInformationBuffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 100 * sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX));

        if (systemLogicalInformationBuffer == NULL) {
            throw std::runtime_error("Could not allocate memory to hold the system logical processor info");
        };

        DWORD bufferSize = 100 * sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX);

        std::cout << "Buffersize: " << bufferSize << std::endl;

        if (!GetLogicalProcessorInformationEx(RelationCache, systemLogicalInformationBuffer, &bufferSize)) {
            throw std::runtime_error("Could not get processor info");
        };

        char* bufferPtr = (char*) systemLogicalInformationBuffer;
        
        size_t i = 0;
        while (i < bufferSize) {
            PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX processInfo = ((PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(bufferPtr + i));
            CACHE_RELATIONSHIP cacheInfo = processInfo->Cache;
            if ((DWORD)cacheInfo.Level == 1) {
                cacheLineSize = (size_t) cacheInfo.LineSize;
                break;
            };
            i += processInfo->Size;
        };

        HeapFree(GetProcessHeap(), 0, systemLogicalInformationBuffer);

        size_t lowerBoundary = reinterpret_cast<size_t>(sysInfo->lpMinimumApplicationAddress);
        size_t allocationGranularity = sysInfo->dwAllocationGranularity;
        size_t startingAddressExponent = BLOCK_EXPONENT;

        std::cout << "Smallest address: " << lowerBoundary << std::endl;

        while (((size_t)1 << startingAddressExponent) <= lowerBoundary) {
            startingAddressExponent++;
        };

        std::cout << "Required starting address exponent to go over minimum address: " << startingAddressExponent << std::endl;

        // Assuming all blocks, pages and granularities are powers of two
        size_t granularityExponent = 0;
        while (allocationGranularity != 1) {
            granularityExponent++;
            allocationGranularity >>= 1;
        };

        std::cout << "Granularity exponent: " << granularityExponent << std::endl;

        while (granularityExponent > startingAddressExponent) {
            startingAddressExponent++;
        }

        std::cout << "Final starting address exponent: " << startingAddressExponent << std::endl;

        size_t PageSize = sysInfo->dwPageSize;

        pageSize = PageSize;

        PVOID start = (PVOID) ((ULONG_PTR) sysInfo->lpMinimumApplicationAddress);
        PVOID end = (PVOID) ((ULONG_PTR) sysInfo->lpMaximumApplicationAddress);

        HeapFree(GetProcessHeap(), 0, sysInfo);

        std::cout << "Trying to allocate: " << initialSize << std::endl;
        
        MEM_ADDRESS_REQUIREMENTS addressReqs;
        addressReqs.Alignment = (SIZE_T) (startingAddressExponent); //(SIZE_T) BLOCK_SIZE;
        addressReqs.LowestStartingAddress = NULL;// (PVOID) ((ULONG_PTR)1 << startingAddressExponent);
        addressReqs.HighestEndingAddress = NULL;

        MEM_EXTENDED_PARAMETER memExtendParam;
        memExtendParam.Type = MemExtendedParameterAddressRequirements;
        memExtendParam.Pointer = &addressReqs;

        // Need to map entire usable address space to page map. 

        std::cout << "Range is 0 to: " << end << std::endl;

        size_t blockCount = ((SIZE_T) end) >> BLOCK_EXPONENT;

        std::cout << "This requires " << blockCount << " blocks in the page map" << std::endl;

        std::cout << "The size of a page is: " << PageSize << std::endl;

        size_t pageExponent = 0;
        while (PageSize >> pageExponent != 1) {
            pageExponent++;
        };

        size_t pageCount = blockCount >> pageExponent;

        std::cout << "Page exponent is: " << pageExponent << std::endl;

        std::cout << "In a single page can fit information for " << PageSize << " blocks, which means we need " << pageCount << " pages" << std::endl; 

        std::cout << "The page map needs stores an offset for every page, so we need a: " <<  pageCount << " sized map" << std::endl;

        std::cout << "We can put this information in pages that are allocated from the returned allocated variable up to the block size boundary for alignment" << std::endl;

        LPVOID initial2GBRegion = VirtualAlloc(NULL, (SIZE_T) initialSize, MEM_RESERVE, PAGE_NOACCESS);// memExtendParam, (ULONG)1 );
        //PVOID initial2GBRegion = VirtualAlloc2(NULL, NULL, (SIZE_T) BLOCK_SIZE, MEM_RESERVE, PAGE_NOACCESS, &memExtendParam, (ULONG)1);
        // THIS IS HOW TO DO POINTER CONVERSION: (PVOID)(ULONG_PTR) 0x7fffffff;
        if (initial2GBRegion == NULL) {
            std::cout << GetLastError() << std::endl;
            std::cout << "Couldnt allocate at " << initial2GBRegion << std::endl;
            //throw std::runtime_error("Could not reserve memory");
        };

        reservedArea = initial2GBRegion;

        std::cout << "Need to create the two level page map now" << std::endl;

        // Initial alloc count:
        // Bytes needed for the higher level page map + bytes needed for each page + bytes needed for allocator for each core count 
        size_t initialCommitCount = pageCount + pageCount * pageSize + coreCount * sizeof(Allocator);

        LPVOID pageMapAddress = VirtualAlloc(initial2GBRegion, initialCommitCount, MEM_COMMIT, PAGE_READWRITE);

        pageMap = pageMapAddress;

        std::cout << "page map address: " << pageMapAddress << std::endl;

        std::cout << "Don't care about the space for the pages, just need to move the offset pointer for allocations now" << std::endl;
        
        std::cout << "Reserved 2GB at: " << initial2GBRegion << std::endl;

        PCHAR pageMapPagesAddress = (PCHAR) pageMapAddress + pageCount;

        pageMapArea = pageMapPagesAddress; 

        // Allocation starts at pageMapPagesAddress + all pages size

        std::cout << "Page map leaves start at: " << (PVOID) pageMapPagesAddress << std::endl;

        PCHAR allocateStartAddress = pageMapPagesAddress + pageCount * PageSize;

        std::cout << "Allocation start: " << (PVOID) allocateStartAddress << std::endl;

        // Need to create pointers to all allocators now 

        for (size_t alloc = 0; alloc < coreCount; alloc++) {
            // make a new allocator in this region 
            new (allocateStartAddress + alloc * sizeof(Allocator)) Allocator;
        };

        allocateStartAddress += coreCount * sizeof(Allocator);

        // Now, need to find start of blocks for block allocator 

        PCHAR blockAllocatorStart = 0;

        while (blockAllocatorStart < allocateStartAddress) {
            blockAllocatorStart += BLOCK_SIZE;
        };

        std::cout << "Block allocator start (aligned): " << (PVOID) blockAllocatorStart << std::endl;

        allocationArea = blockAllocatorStart;
        currentAllocBumpPtr = 0;

        // Need to mark used blocks now 

        std::cout << "Page map does not include meta data" << std::endl;

        // Now need to calculat sizes for all size classes 
        
        uint8_t sc = 0;
        // need to avoid overflow here 
        while ((sizeClassSizes[sc - 1] < BLOCK_SIZE) && (sc < 255)) {
            sizeClassSizes[sc] = sizeClassToSize(sc);
            sc++;
        };
        sizeClassSizes[sc] = sizeClassToSize(sc);    

        for (size_t j = 0; j < 256; j++) {
            std::cout << "For size class " << j << " size is: " << sizeClassSizes[j] << std::endl;
        };
    };

    size_t WdMalloc::sizeClassToSize(uint8_t sizeClass) {
        sizeClass++;
        // get first 2 bits 
        size_t m = sizeClass & 3;
        size_t e = sizeClass >> 2; // exponent is next 6 bits 
        size_t b = (e == 0) ? 0 : 1;
        // all sizes should be 2 byte aligned 
        return (m + 4 * b) << (4 + e - b);
    };

    uint8_t WdMalloc::sizeToSizeClass(size_t size) {
        size--;
        size_t e = 58 - __lzcnt64(size | 32);
        size_t b = (e == 0) ? 0 : 1;
        size_t m = (size >> (4 + e - b)) & 3;
        return (e << 2) + m;
    };

    void WdMalloc::mallocShutdown() {
        if (!VirtualFree(reservedArea, 0, MEM_RELEASE)) {
            throw std::runtime_error("Could not free memory");
        };
    };

    void WdMalloc::SendToDeallocQueue(Allocator* allocator, DeallocMessage *first, volatile DeallocMessage *last, size_t addedSize) {
        // Determine which queue we want to use, the home bucket of the allocator 
        size_t queueIndex = (size_t) (allocator) & INITIAL_BIT_MASK;
        last->next = nullptr; // Make sure this holds
        DeallocMessage *previousBack = static_cast<DeallocMessage *>(InterlockedExchangePointer((volatile PVOID*) &(allocator->deallocQueues[queueIndex].end), (PVOID) last));
        previousBack->next = first;
        // Sizing, atomic loop for now? 
        size_t oldSize = allocator->currentDeallocSize;
        while (InterlockedCompareExchange(&allocator->currentDeallocSize, oldSize + addedSize, oldSize) != oldSize) {
            oldSize = allocator->currentDeallocSize;
        };
    };

    WdMalloc::DeallocMessage *WdMalloc::DequeueDeallocQueue(DeallocMessage *first) {
        // For the async enqueus to work, we always need the queue to be non empty 
        if (first->next == nullptr) {
            return nullptr;
        };
        // This is safe to do since we are guaranteed a next exists, and the enqueues only modify the back pointer 
        DeallocMessage *front = first;
        first = first->next;
        return front;
    };

    void WdMalloc::flushDeallocRequests() {
        Allocator* allocator; // thread local storage 

        size_t addressMask = INITIAL_BIT_MASK;
        size_t bits = 0;
        bool emptyQueues = false;
        while (!emptyQueues) {
            // Do this until home queue is empty, which means everything has been distributed 
            size_t homeBucket = ((size_t) (allocator) >> bits) & addressMask;
            for (size_t i = 0; i < MALLOC_BUCKETS; ++i) {
                // if not home bucket, send 
                if (i != homeBucket) {
                    SendToDeallocQueue(allocator->deallocQueues[i].first->parentAlloc, allocator->deallocQueues[i].first, allocator->deallocQueues[i].end, allocator->deallocQueues[i].size);
                    // Send all non empty queues to respective allocators, except home bucket  
                    allocator->deallocQueues[i].first = nullptr;
                    allocator->deallocQueues[i].end = nullptr;
                };
            };
            bits+= KEY_BITS;
            // Now, need to reshuffle home bucket 
            DeallocMessage *ptr = allocator->deallocQueues[homeBucket].first;
            if (ptr == nullptr) {
                emptyQueues = true;
            };
            while (ptr != nullptr) {
                // Allocator owns the memory designated by the pointer 
                DeallocMessage* next = ptr->next;
                if (ptr->parentAlloc == allocator) {
                    freeInternal(ptr);
                } else {
                    allocator->deallocQueues[((size_t) (ptr->parentAlloc) >> bits) & addressMask].end->next = ptr;
                };
                ptr = next;
            }; 
        };
        // everything should be assigned now 
    };

    void WdMalloc::free(void* ptr) {
        Allocator* allocator; // thread local storage
        // add to bucket 
        // Need to look in page map to find things 
        // and use slabs alll that 
        //allocator->deallocQueues[]
    };

    void *WdMalloc::malloc(size_t size) {
        return nullptr;
    };

    void WdMalloc::freeInternal(void *ptr) {

    };

};