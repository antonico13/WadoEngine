#ifndef WD_MALLOC_H
#define WD_MALLOC_H

#include <stdint.h>
#include <cstdint>

#define BYTE_TO_BITS 8

namespace Wado::Malloc {

    // Types:
    using size_class = uint8_t;

    // block size, also used to calculate starting address.
    static const size_t BLOCK_EXPONENT = 21;
    static const size_t TWO_GIGABYTE_EXPONENT = 31;
    static const size_t BLOCK_SIZE = (size_t)1 << BLOCK_EXPONENT;
    static const size_t TWO_GIGABYTE = (size_t)1 << TWO_GIGABYTE_EXPONENT; // can fit 2^10 chunks in this allocation 
    static const size_t MEDIUM = 4096; // 4kb, should just make it page size maybe 


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
            // Objects > block size -> large objects
            // Objects <= block size but > mediumBoundary -> medium
            // Objects <= mediumBoundary => medium 
            static size_t blockSize;
            static size_t mediumBoundary;
            static size_t coreCount;

            static size_t areaSize;
            // reserved Area is also the start of the higher level page map area 
            static uintptr_t reservedArea;

            // page map area is split up into pages 
            static uintptr_t pageMap;
            static uintptr_t pageMapArea;
            static size_t pageSize; 
            static size_t pageExponent;

            // start of allocation area 
            static uintptr_t allocationArea;
            // Bump pointer for current allocation (everything block based)
            static volatile size_t currentAllocBumpPtr;
            static volatile uint64_t allocationLock;

            static const uint64_t LOCKED = 1;
            static const uint64_t UNLOCKED = 0;

            static uintptr_t allocatorArea;
            static size_t sizeClassSizes[255];
            
            using LargeStackNode = struct LargeStackNode {
                volatile LargeStackNode *next;
            };

            static volatile LargeStackNode *largeStacks[BYTE_TO_BITS * sizeof(void *)];

            static size_t MEDIUM_ALLOC;
            static size_t LARGE_ALLOC;

            // Some size class utils 

            static size_t sizeClassToSize(uint8_t sizeClass);
            static uint8_t sizeToSizeClass(size_t size);

            enum BlockType {
                Free = 0x00,
                Super = 0x01,
                Medium = 0x02,
                Unused = 0x03,
                // For jump, we need n = addressable elements exponent - block exponent, and we have Jump[0,...n) and Large[0,....n). then
                // the large will be bx01<n bits>, and jump will be bx10<nbits>, so n has to be at most 6 for this design to work
            };

            static const size_t KEY_BITS = 6;
            static const size_t MALLOC_BUCKETS = (size_t)1 << KEY_BITS;
            static const size_t DEALLOC_THRESHOLD = ((size_t)1 << 17);
            static const size_t INITIAL_BIT_MASK = MALLOC_BUCKETS - 1; // 2^k - 1 to get first k bits all set to 1 

            static const size_t ALLOC_THRESHOLD = BLOCK_SIZE >> 1; 

            struct Allocator;

            using DeallocMessage = struct DeallocMessage {
                Allocator* parentAlloc;
                DeallocMessage *next;
            };

            using DeallocQueue = struct DeallocQueue {
                DeallocMessage *first;
                volatile DeallocMessage *end;
            };

            using DLLNode = struct DLLNode {
                void *prev;
                void *next;
            };

            using Allocator = struct Allocator {
                // The allocator. 
                // TOOD: should this be different so other allocators can check this? 
                size_t currentDeallocSize = 0;
                DeallocQueue deallocQueues[MALLOC_BUCKETS];
                size_t currentBitMask = INITIAL_BIT_MASK;

                void* superBlocks;

                // array of pointer to blocks of all possible size classes for
                // this alloc, which are traversable DLLs
                void* blocks[255];
            };

            using BlockMetaData = struct BlockMetaData {
                void *allocator;
                BlockType type;
            };

            using SmallBlockMetadata = struct SmallBlockMetaData {
                uint8_t used;
                union {
                    uint8_t next;
                    struct {
                        uint8_t head;
                        uint8_t link;
                        uint8_t sizeClass;
                    } usedMetaData;
                } data;
            };

            using SuperBlockMetadata = struct SuperBlockMetadata {
                DLLNode dllNode;
                uint16_t freeCount;
                uint8_t head;
                uint8_t used;
                SmallBlockMetadata smallBlocks[255]; 
            };

            using MediumBlockMetadata = struct MediumBlockMetadata {
                DLLNode dllNode;
                uint16_t freeCount;
                uint8_t head;
                uint8_t sizeClass;
                struct {
                    uint8_t next;
                    uint8_t object;
                } freeStack[256];
            };

            using MediumBlock = struct MediumBlock {                
                // DLL offset would be cache line size 

                static const size_t DLL_NODE_SIZE = 2 * sizeof(void *); // DLL node for same size class, same owner free medium slabs 
                static const size_t FREE_COUNT_SIZE = sizeof(uint16_t); // 16 bit free count 
                static const size_t HEAD_SIZE = sizeof(uint8_t);
                static const size_t CLASS_SIZE = sizeof(uint8_t);

                // then free stack follows, a linked list of unused objects 
            };

            // Superblocks contain many small blocks. A small block is the size of a page 
            using SuperBlock = struct SuperBlock {
                static const size_t DLL_NODE_SIZE = 2 * sizeof(void *); // DLL node for same size class, same owner free medium slabs 
                static const size_t FREE_COUNT_SIZE = sizeof(uint16_t); // 16 bit free count 
                static const size_t HEAD_SIZE = sizeof(uint8_t); // index into metadata array 
                static const size_t CLASS_SIZE = sizeof(uint8_t); // this is the size of the next/size class small meta element. since we index a 255 sized arary, fine to keep it 8 bit 
                static const size_t USED_SIZE = sizeof(uint8_t);
                static const size_t LINK_SIZE = sizeof(void *);

                static const size_t SMALL_META_SIZE = USED_SIZE + HEAD_SIZE + LINK_SIZE + sizeof(void *); 
            };

            static void SendToDeallocQueue(Allocator* allocator, DeallocMessage *first, volatile DeallocMessage *last, size_t addedSize);

            static DeallocMessage *DequeueDeallocQueue(DeallocMessage *first);

            static void flushDeallocRequests();

            static size_t getPtrSizeMedium(void *ptr);

            static size_t getPtrSizeSmall(void *ptr);

            static void freeInternal(void *ptr); 

            static void freeInternalMedium(void *ptr); 

            static void freeInternalSmall(void *ptr); 

            static void freeInternalLarge(void *ptr);

            static void InitializeSuperBlock(uint8_t sizeClass, void *address);

            static void *allocSmall(size_t size);

            static void InitializeMediumBlock(uint8_t sizeClass, void *address);

            static void *allocMedium(size_t size);

            static void *allocLarge(size_t size);
            
            // By default get and commit just one block 
            static void *getBlocks(const size_t blockCount = 1, const size_t commitSize = BLOCK_SIZE);

            // Need to mask out first 21 bits 
            static const size_t blockOffsetMask = ~((size_t)1 << 21 - 1);
            static const size_t blockOffsetExponent = 21;
            static void registerBlock(void *blockAddress, BlockType type);
            static void registerBlocks(void *blockAddress, size_t blockCount, int type);


            static void pushToLargeStack(LargeStackNode *node, const size_t exponent);
            static void *popLargeStack(const size_t exponent);
    };
};

#endif