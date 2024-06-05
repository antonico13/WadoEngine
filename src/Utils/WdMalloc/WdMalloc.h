#ifndef WD_MALLOC_H
#define WD_MALLOC_H

namespace Wado::Malloc {

    // block size, also used to calculate starting address.
    static const size_t BLOCK_EXPONENT = 21;
    static const size_t TWO_GIGABYTE_EXPONENT = 31;
    static const size_t BLOCK_SIZE = (size_t)1 << BLOCK_EXPONENT;
    static const size_t TWO_GIGABYTE = (size_t)1 << TWO_GIGABYTE_EXPONENT; // can fit 2^10 chunks in this allocation 


    class WdMalloc {
        public: 

            static void mallocInit(const size_t initialSize, const size_t coreCount);

            static void mallocShutdown();

            static void free(void* ptr);

            static void *malloc(size_t size);

        private: 
            WdMalloc() = delete;
            ~WdMalloc() = delete;

            // Sys info stuff 
            static size_t cacheLineSize;
            static size_t coreCount;

            static size_t areaSize;
            // reserved Area is also the start of the higher level page map area 
            static void *reservedArea;

            // page map area is split up into pages 
            static void *pageMapArea;
            static size_t pageSize; 

            // start of allocation area 
            static void *allocationArea;
            // Bump pointer for current allocation (everything block based)
            static size_t currentAllocBumpPtr;

            enum BlockType {
                Free = 0x00,
                Super = 0x01,
                Medium = 0x02,
                Unused = 0x03,
                // For jump, we need n = addressable elements exponent - block exponent, and we have Jump[0,...n) and Large[0,....n). then
                // the large will be  bx01<n bits>, and jump will be bx10<nbits>, so n has to be at most 6 for this design to work
            };

            static const size_t KEY_BITS = 6;
            static const size_t MALLOC_BUCKETS = (size_t)1 << KEY_BITS;
            static const size_t DEALLOC_THRESHOLD = ((size_t)1 << 17);
            static const size_t INITIAL_BIT_MASK = MALLOC_BUCKETS - 1; // 2^k - 1 to get first k bits all set to 1 

            struct Allocator;

            using DeallocMessage = struct DeallocMessage {
                Allocator *parentAlloc;
                DeallocMessage *next;
            };

            using DeallocQueue = struct DeallocQueue {
                DeallocMessage *first;
                volatile DeallocMessage *end;
                volatile size_t size = 0;
            };

            using Allocator = struct Allocator {
                // The allocator. 
                volatile size_t currentDeallocSize = 0;
                DeallocQueue deallocQueues[MALLOC_BUCKETS];
                size_t currentBitMask = INITIAL_BIT_MASK;
            };

            static void SendToDeallocQueue(Allocator* allocator, DeallocMessage *first, volatile DeallocMessage *last, size_t addedSize);

            static DeallocMessage *DequeueDeallocQueue(DeallocMessage *first);

            static void flushDeallocRequests();

            static void freeInternal(void *ptr); 
    };
};

#endif