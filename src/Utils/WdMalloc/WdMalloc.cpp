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
        uintptr_t WdMalloc::reservedArea;

            // page map area is split up into pages 
        uintptr_t WdMalloc::pageMap;
        uintptr_t WdMalloc::pageMapArea;
        size_t WdMalloc::pageSize; 
        size_t WdMalloc::pageExponent;

        size_t WdMalloc::MEDIUM_ALLOC;
        size_t WdMalloc::LARGE_ALLOC;

        // start of allocation area 
        uintptr_t WdMalloc::allocationArea;
        // Bump pointer for current allocation (everything block based)
        volatile size_t WdMalloc::currentAllocBumpPtr;

        uintptr_t WdMalloc::allocatorArea;
        size_t WdMalloc::sizeClassSizes[255];
        

    // Aligns a pointer up to the specified size, in bytes 
    // Pre: size is a power of 2 
    inline uintptr_t alignUp(uintptr_t ptr, size_t size) {
        uintptr_t sizeMask = ~(size - 1);
        std::cout << "Size mask being used to round up: " << (void *) sizeMask << std::endl;
        return (ptr + size) & sizeMask;
    };

    // Aligns a pointer down to the specified size, in bytes 
    // Pre: size is a power of 2
    inline uintptr_t alignDown(uintptr_t ptr, size_t size) {
        uintptr_t sizeMask = ~(size - 1);
        return ptr & sizeMask;
    };

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
        MEDIUM_ALLOC = PageSize;
        LARGE_ALLOC = BLOCK_SIZE;

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

        size_t PageExponent = 0;
        while (PageSize >> PageExponent != 1) {
            PageExponent++;
        };
        pageExponent = PageExponent;

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

        reservedArea = reinterpret_cast<uintptr_t>(initial2GBRegion);

        std::cout << "Need to create the two level page map now" << std::endl;

        // Initial alloc count:
        // Bytes needed for the higher level page map + bytes needed for each page + bytes needed for allocator for each core count 

        LPVOID pageMapAddress = VirtualAlloc(initial2GBRegion, pageCount, MEM_COMMIT, PAGE_READWRITE);

        pageMap = reinterpret_cast<uintptr_t>(pageMapAddress);

        std::cout << "page map address: " << (void *) pageMapAddress << std::endl;

        std::cout << "Don't care about the space for the pages, just need to move the offset pointer for allocations now" << std::endl;
        
        std::cout << "Reserved 2GB at: " << (void *) initial2GBRegion << std::endl;

        pageMapArea = pageMap + pageCount;

        // Allocation starts at pageMapPagesAddress + all pages size

        std::cout << "Page map leaves start at: " << (void *) pageMapArea << std::endl;

        allocatorArea = pageMapArea + pageCount * PageSize;

        std::cout << "Allocation start: " << (void *) allocatorArea << std::endl;
        // Commit allocators 
        if (VirtualAlloc((void *) allocatorArea, coreCount * sizeof(Allocator), MEM_COMMIT, PAGE_READWRITE) == NULL) {
            std::cout << "Could not commit allocator space: " << GetLastError() << std::endl;
            throw std::runtime_error("Could not commit allocator space");
        };

        std::cout << "Comitted allocator pages" << std::endl;
        // Need to create pointers to all allocators now 
        for (size_t alloc = 0; alloc < coreCount; alloc++) {
            // make a new allocator in this region 
            new (reinterpret_cast<void *> (allocatorArea + alloc * sizeof(Allocator))) Allocator;
        };

        uintptr_t allocatorEnd =  allocatorArea + coreCount * sizeof(Allocator);

        // Now, need to find start of blocks for block allocator 

        allocationArea = 0;

        while (allocationArea < allocatorEnd) {
            allocationArea += BLOCK_SIZE;
        };

        std::cout << "Block allocator start (aligned): " << (void *) allocationArea << std::endl;

        currentAllocBumpPtr = 0;

        // Need to mark used blocks now 

        std::cout << "Page map does not include meta data" << std::endl;

        // Now need to calculat sizes for all size classes 
        
        uint8_t sc = 0;
        // need to avoid overflow here 
        sizeClassSizes[0] = sizeClassToSize(sc);
        sc++;
        while ((sizeClassSizes[sc - 1] < BLOCK_SIZE) && (sc < 255)) {
            sizeClassSizes[sc] = sizeClassToSize(sc);
            sc++;
        };

        for (size_t j = 0; j < 255; j++) {
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
        if (!VirtualFree(reinterpret_cast<LPVOID>(reservedArea), 0, MEM_RELEASE)) {
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
        Allocator* alloc = reinterpret_cast<Allocator *>(allocatorArea);
        uintptr_t block = alignDown(reinterpret_cast<uintptr_t>(ptr), BLOCK_SIZE);

        std::cout << "For pointer " << ptr << " aligned down to block pointer: " << (void *) block << std::endl;

        uintptr_t parentAllocPtr = *reinterpret_cast<uintptr_t *>(block);

        std::cout << "Parent allocator pointer is: " << (void *) parentAllocPtr << std::endl;

        size_t bucketId = parentAllocPtr & INITIAL_BIT_MASK;

        DeallocMessage *msg = reinterpret_cast<DeallocMessage *>(ptr);
        msg->next = nullptr;
        msg->parentAlloc = reinterpret_cast<Allocator *>(parentAllocPtr);

        alloc->deallocQueues[bucketId].end->next = msg;

        // TODO: how to get size here? 
    };

    void *WdMalloc::malloc(size_t size) {
        Allocator* alloc = reinterpret_cast<Allocator *>(allocatorArea);
        uint8_t sizeClass = sizeToSizeClass(size);
        std::cout << "Alloc address: " << alloc << std::endl;
        std::cout << "Looking for an object of sizeclass: " << (int) sizeClass << std::endl;
        if (size > LARGE_ALLOC) {
            // TODO:
            return nullptr;
        };
        if (size > MEDIUM_ALLOC) {
            std::cout << "Got a medium allocation " << std::endl;
            return allocMedium(size);
        };

        std::cout << "Got a small allocation" << std::endl;

        return allocSmall(size);
    };

    void WdMalloc::InitializeSuperBlock(uint8_t sizeClass, void *address) {
        // 255 small blocks in a super block, with the first one taking 
        // metadata space 
        Allocator* alloc = reinterpret_cast<Allocator *>(allocatorArea);
        BlockMetaData *blockMetaData = static_cast<BlockMetaData *>(address);
        blockMetaData->allocator = alloc;
        blockMetaData->type = BlockType::Super;

        void *startAddress = reinterpret_cast<void *>((uintptr_t) (address) + cacheLineSize);
        SuperBlockMetadata *superBlockMetadata = static_cast<SuperBlockMetadata *>(startAddress);
        superBlockMetadata->dllNode.prev = 0;
        superBlockMetadata->dllNode.next = 0;

        superBlockMetadata->freeCount = (BLOCK_SIZE >> pageExponent) - 1;
        superBlockMetadata->head = 1; // half block is not free
        superBlockMetadata->used = 1; // half block is used

        for (int i = 1; i < 254; i++) {
            superBlockMetadata->smallBlocks[i].used = 0;
            superBlockMetadata->smallBlocks[i].data.next = i + 1;
        };

        superBlockMetadata->smallBlocks[255].used = 0;
        superBlockMetadata->smallBlocks[255].data.next = 0;

        // However, slab 0 is actually being used right now,
        // to host element of size sizeclass 
        // need to set up its bump pointer, free list, etc

        // First block, allocation starts from cache line size + superblock metadata
        superBlockMetadata->smallBlocks[0].used = 1; // we consider the metadata one object in this case 
        superBlockMetadata->smallBlocks[0].data.usedMetaData.sizeClass = sizeClass;

        size_t metaDataSize = cacheLineSize + sizeof(SuperBlockMetadata);
        size_t smallBlockObjectSize = sizeClassSizes[sizeClass];

        uintptr_t metaDataEnd = metaDataSize + (uintptr_t) (address);

        std::cout << "Meta data ends at: " << (void *) metaDataEnd << std::endl;

        uintptr_t firstObject = alignUp(metaDataEnd, smallBlockObjectSize);

        std::cout << "First object starts at: " << firstObject << std::endl;

        uint8_t headIndex = firstObject >> (__lzcnt64(smallBlockObjectSize));

        std::cout << "This object is the " << (int) headIndex << "th element" << std::endl;
        // Can set allocation start for this slab now 
        // Bump pointer if index at that location is itself 
        superBlockMetadata->smallBlocks[0].data.usedMetaData.head = headIndex;

        std::cout << "Also need to set the bump pointer for the first object" << std::endl;

        *reinterpret_cast<uint8_t *>(firstObject) = headIndex;

        uint8_t linkIndex = (pageSize >> (__lzcnt64(smallBlockObjectSize))) - 1;

        std::cout << "The link index is: " << (int) linkIndex << std::endl;
        superBlockMetadata->smallBlocks[0].data.usedMetaData.link = linkIndex; // last element of the small block
    };


    void *WdMalloc::allocSmall(size_t size) {
        // First, look for a block of the requested size
        Allocator *allocator = reinterpret_cast<Allocator *>(allocatorArea);
        uint8_t sizeClass = sizeToSizeClass(size);
        size_t roundedSize = sizeClassSizes[sizeClass];
        
        if (allocator->blocks[sizeClass] == nullptr) {
            // not found;
            std::cout << "Couldn't find block for the specified size" << std::endl;
            void *smallBlockAddr;
            if (allocator->superBlocks == nullptr) {
                std::cout << "All allocator superblocks are empty, making a new one" << std::endl;
                void *blockAddr = getBlock();
                std::cout << "Reserved a block at: " << blockAddr << std::endl;
                // Need to mark on page map now 
                registerBlock(blockAddr, BlockType::Super);
                std::cout << "Registered block as a super block" << std::endl;
                InitializeSuperBlock(sizeClass, blockAddr);

                // up to the first metadata
                allocator->superBlocks = blockAddr;
                smallBlockAddr = reinterpret_cast<void*>((uintptr_t) blockAddr + cacheLineSize + sizeof(DLLNode) + sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint8_t));
            } else {
                SuperBlockMetadata* superBlock = reinterpret_cast<SuperBlockMetadata *>((uintptr_t) allocator->superBlocks + cacheLineSize);
                
                // Guaranteed not to be empty 
                uint8_t freeIndex = superBlock->head;
                SmallBlockMetaData& smallBlock = superBlock->smallBlocks[freeIndex];

                std::cout << "Found superblock, has free small block with index: " << (int) freeIndex << std::endl;
                std::cout << "Next free one is: " << (int) smallBlock.data.next << std::endl;

                superBlock->freeCount--;
                superBlock->head = smallBlock.data.next;

                if (superBlock->freeCount == 0) {
                    std::cout << "This super block is fully empty now" << std::endl;
                    allocator->superBlocks = superBlock->dllNode.next;
                    std::cout << "Replaced superblock pointer with next pointer: " << allocator->superBlocks << std::endl;
                };
                
                smallBlockAddr = &(superBlock->smallBlocks[freeIndex]);

                std::cout << "Setting up bump pointer for small block" << std::endl;

                smallBlock.used = 1;
                smallBlock.data.usedMetaData.sizeClass = sizeClass;
                smallBlock.data.usedMetaData.head = 0;
                smallBlock.data.usedMetaData.link = (pageSize - roundedSize) >> __lzcnt(roundedSize);
                
                std::cout << "Setting bump pointer now" << std::endl;
                *reinterpret_cast<uint8_t *>((uintptr_t) superBlock + freeIndex * pageSize) = 0;
            };
            std::cout << "Initialized the super block at this memory block and got small block at: " << smallBlockAddr << std::endl;
            allocator->blocks[sizeClass] = smallBlockAddr;
        };

        SmallBlockMetaData *metadata = reinterpret_cast<SmallBlockMetaData *>(allocator->blocks[sizeClass]);

        std::cout << "Got small block metadata at: " << metadata << std::endl;

        uintptr_t superBlockAddress = alignDown((uintptr_t) metadata, BLOCK_SIZE);

        std::cout << "Superblock address is: " <<  (void *) superBlockAddress << std::endl;

        uintptr_t metaStartAddress = superBlockAddress + cacheLineSize + sizeof(DLLNode) + sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint8_t);

        uint8_t metaIndex = (reinterpret_cast<uintptr_t>(metadata) - metaStartAddress) >> __lzcnt(sizeof(SmallBlockMetaData));

        std::cout << "Index of this metadata is: " << (int) metaIndex << std::endl;

        uintptr_t smallBlockStart = superBlockAddress + metaIndex * pageSize;

        std::cout << "The small block for this metadata starts at: " << (void *) smallBlockStart << std::endl;

        uintptr_t allocatedAddress = smallBlockStart + metadata->data.usedMetaData.head * roundedSize;

        uint8_t listHead = *reinterpret_cast<uint8_t *>(allocatedAddress);

        if (listHead == metadata->data.usedMetaData.head) {
            std::cout << "Bump pointer allocation" << std::endl;
            metadata->data.usedMetaData.head++;
            std::cout << "New head is: " << (int) metadata->data.usedMetaData.head << std::endl;
            *reinterpret_cast<uint8_t *>(allocatedAddress + roundedSize) = metadata->data.usedMetaData.head;
        } else {
            std::cout << "Free list allocation" << std::endl;
            std::cout << "New head is: " << (int) listHead << std::endl;
            metadata->data.usedMetaData.head = listHead;
        };

        std::cout << "Now checking if allocation has been filled " << std::endl;


        if (metadata->data.usedMetaData.head == -1) {
            std::cout << "Head has been filled " << std::endl;

            std::cout << "Setting new value from linked list node, which must be the fully allocated address now " << std::endl;

            DLLNode *node = reinterpret_cast<DLLNode *>(allocatedAddress);

            allocator->blocks[sizeClass] = node->next;
        };

        return reinterpret_cast<void *>(allocatedAddress);
    };

    void *WdMalloc::getBlock() {
        // Need to increment this to ensure we have a saved block 
        // TODO: this will need to be a free list
        InterlockedIncrement(&currentAllocBumpPtr);
        uintptr_t blockPtr = allocationArea + ((currentAllocBumpPtr - 1) << BLOCK_EXPONENT);
        if (VirtualAlloc((PVOID) blockPtr, BLOCK_SIZE, MEM_COMMIT, PAGE_READWRITE) == NULL) {
            std::cout << "Could not commit block memory" << std::endl;
            throw std::runtime_error("Could not commit block memory");
        };
        return reinterpret_cast<void *>(blockPtr);
    };
    
    void WdMalloc::registerBlock(void *blockAddress, BlockType type) {
        size_t blockOffset = ((uintptr_t)(blockAddress) & blockOffsetMask) >> blockOffsetExponent;
        std::cout << "For block at " << blockAddress << " offset is " << blockOffset << std::endl;
        size_t pageOffset = blockOffset >> pageExponent;
        std::cout << "For this block, the page offset is: " << pageOffset << std::endl;
        size_t inPageOffset = blockOffset & (((size_t) 1 << pageExponent) - 1);
        std::cout << "For this block, the in page offset is: " << inPageOffset << std::endl;

        uintptr_t pageAddress = pageMapArea + pageOffset * pageSize;

        std::cout << "Address of the requested page is: " << (void *) pageAddress << std::endl;

        if ( *reinterpret_cast<char *>(pageMap + pageOffset) == 0) {
            std::cout << "Page storing this block's details has not been committed yet" << std::endl;

            std::cout << "Committing page at: " << (void *) pageAddress << std::endl;
            LPVOID page = VirtualAlloc((void *) pageAddress, pageSize, MEM_COMMIT, PAGE_READWRITE);
            if (page == nullptr) {
                std::cout << "Could not commit page" << std::endl;
                throw std::runtime_error("Could not commit page map page");
            };
            *(reinterpret_cast<char *>(pageMap + pageOffset)) = 1;
        };
        
        *(reinterpret_cast<char *>(pageAddress + inPageOffset)) = type;
    };

    
    void *WdMalloc::allocMedium(size_t size) {
        Allocator *allocator = reinterpret_cast<Allocator *>(allocatorArea);
        uint8_t sizeClass = sizeToSizeClass(size);
        if (allocator->blocks[sizeClass] == nullptr) {
            // not found;
            std::cout << "Couldn't find block for the specified size" << std::endl;
            void *blockAddr = getBlock();
            std::cout << "Reserved a block at: " << blockAddr << std::endl;
            // Need to mark on page map now 
            registerBlock(blockAddr, BlockType::Medium);
            std::cout << "Registered block as a medium block" << std::endl;
            InitializeMediumBlock(sizeClass, blockAddr);
            std::cout << "Initialized the medium block at this memory block" << std::endl;
            allocator->blocks[sizeClass] = blockAddr;
        };

        MediumBlockMetadata *metadata = reinterpret_cast<MediumBlockMetadata *>((uintptr_t) allocator->blocks[sizeClass] + cacheLineSize);
        // As a precondition this has to be free 
        uint8_t head = metadata->head;
        // To calculate head, we need to get the next address aligned with the allocation size from here 
        // Mask to align pointers 
        // Sizes are always powers of two 
        uintptr_t startAddress = alignUp((uintptr_t) metadata, sizeClassSizes[sizeClass]);

        std::cout << "Metadata start address: " << metadata << std::endl;
        std::cout << "First object start address after rounding up to object size: " << (void *) startAddress << std::endl;

        std::cout << "Head pointer of free list is: " << (int) head << std::endl;

        std::cout << "Free count for medium block is: " << (int) metadata->freeCount << std::endl;

        uint8_t headObj = metadata->freeStack[head].object;
        uint8_t nextIndex = metadata->freeStack[head].next;

        std::cout << "At head, the free list element is: " << "[" << (int) headObj << ", " << (int) nextIndex << "]" << std::endl;

        uintptr_t objectAddress = startAddress + headObj * sizeClassSizes[sizeClass];

        std::cout << "Object allocated at: " << (void *) objectAddress << std::endl;

        std::cout << "Modifying free list head, and free count" << std::endl;

        metadata->freeCount--;
        metadata->head = nextIndex;

        if (metadata->freeCount == 0) {
            std::cout << "Block is full, removing from list" << std::endl;
            allocator->blocks[sizeClass] = metadata->dllNode.next;
        };

        return reinterpret_cast<void *>(objectAddress);
    };


    void WdMalloc::InitializeMediumBlock(uint8_t sizeClass, void *address) {
        Allocator* alloc = reinterpret_cast<Allocator *>(allocatorArea);
        BlockMetaData *blockMetaData = static_cast<BlockMetaData *>(address);
        blockMetaData->allocator = alloc;
        blockMetaData->type = BlockType::Medium;

        uintptr_t startAddress = (reinterpret_cast<uintptr_t>(address) + cacheLineSize);
        MediumBlockMetadata *mediumBlockMetaData = reinterpret_cast<MediumBlockMetadata *>(startAddress);

        std::cout << "After adding cache line size, recording medium block metadata at: " << (void *) mediumBlockMetaData << std::endl;
        mediumBlockMetaData->dllNode.next = 0;
        mediumBlockMetaData->dllNode.prev = 0;
        mediumBlockMetaData->head = 0;
        mediumBlockMetaData->sizeClass = sizeClass;

        // first object unused because we store metadata there 
        uint16_t objectCount = BLOCK_SIZE / sizeClassSizes[sizeClass] - 1; // TODO: replace with bit shift
        mediumBlockMetaData->freeCount = objectCount;
        std::cout << "For the requested size, can fit " << objectCount << " objects in the medium block" << std::endl;
        std::cout << "Free count is: " << (int) mediumBlockMetaData->freeCount << std::endl;
        // Initial setup, everything is free and the free stack 
        // contains a pointer to the next element in the free stack 
        // and its object always 
        for (size_t i = 0; i < objectCount; i++) {
            mediumBlockMetaData->freeStack[i].next = i + 1;
            mediumBlockMetaData->freeStack[i].object = i;
        };
        // Mark end of list 
        mediumBlockMetaData->freeStack[objectCount - 1].next = -1;

        std::cout << "Free stack for this block: " << std::endl;
        for (size_t i = 0; i < objectCount; i++) {
            std::cout << "[" << (int) mediumBlockMetaData->freeStack[i].next << ", " << (int) mediumBlockMetaData->freeStack[i].object << "]" << std::endl; 
        };
    };

    void WdMalloc::freeInternal(void *ptr) {

    };

    void WdMalloc::freeInternalMedium(void *ptr) {
        Allocator* alloc = reinterpret_cast<Allocator *>(allocatorArea);
        uintptr_t block = alignDown((uintptr_t) ptr, BLOCK_SIZE);

        MediumBlockMetadata *metaData = reinterpret_cast<MediumBlockMetadata *>(block + cacheLineSize);
        size_t size = sizeClassSizes[metaData->sizeClass];

        std::cout << "We have size " << size << std::endl;

        size_t sizeExponent = __lzcnt64(size);

        std::cout << "With exponent: " << sizeExponent << std::endl;

        uint8_t objectID = (((uintptr_t) ptr - block) >> sizeExponent) - 1; // Need to account for initial metadata object

        std::cout << "This object has id: " << (int) objectID << std::endl;

        metaData->freeStack[objectID].object = objectID;
        metaData->freeStack[objectID].next = metaData->head;

        metaData->head = objectID;

        metaData->freeCount++;

        std::cout << "Created free stack new node" << std::endl;

        if (metaData->freeCount == 1) {
            std::cout << "This object was full before, adding to metadata array for this allocator " << std::endl;
            
            // TODO: Is this good enough? It's a DLL but i'm only using/setting the front
            metaData->dllNode.next = alloc->blocks[metaData->sizeClass];
            // Assign new block  
            alloc->blocks[metaData->sizeClass] = (void *) block;
        };
    };

    void WdMalloc::freeInternalSmall(void *ptr) {
        Allocator* alloc = reinterpret_cast<Allocator *>(allocatorArea);
        uintptr_t block = alignDown((uintptr_t) ptr, BLOCK_SIZE);

        SuperBlockMetadata *metaData = reinterpret_cast<SuperBlockMetadata *>(block + cacheLineSize);

        std::cout << "Freeing small object at superblock: " << (void *) metaData << std::endl;

        uintptr_t objectPage = alignDown((uintptr_t) ptr, pageSize);

        std::cout << "Freeing small object in page that starts at: " << (void *) objectPage << std::endl;

        uint8_t smallBlockMetaIndex = (objectPage - block) >> pageExponent;

        std::cout << "This has metadata index: " << (int) smallBlockMetaIndex << std::endl;
        
        SmallBlockMetaData& smallBlockMetada = metaData->smallBlocks[smallBlockMetaIndex];
        
        std::cout << "The small block has this many used objects: " << (uint8_t) smallBlockMetada.used << std::endl;

        std::cout << "The size class is: " << (uint8_t) smallBlockMetada.data.usedMetaData.sizeClass << std::endl;
        
        size_t size = sizeClassSizes[smallBlockMetada.data.usedMetaData.sizeClass];

        uint8_t previousHead = smallBlockMetada.data.usedMetaData.head;

        std::cout << "Previous head is: " << (int) previousHead << std::endl;

        std::cout << "We have size " << size << std::endl;

        uint8_t objectIndex = ((uintptr_t) ptr - objectPage) >> __lzcnt(size);

        std::cout << "The object being freed is at this index in the page: " << (int) objectIndex << std::endl;

        smallBlockMetada.used--;

        std::cout << "The used count for this small block is now: " << (int) smallBlockMetada.used << std::endl;

        smallBlockMetada.data.usedMetaData.head = objectIndex;

        std::cout << "Updated small block head " << std::endl;

        *reinterpret_cast<uint8_t *>(ptr) = previousHead;

        std::cout << "Updated what is stored in the ptr too" << std::endl;

        if (previousHead == -1) {
            std::cout << "This small block was full before" << std::endl;

            smallBlockMetada.data.usedMetaData.link = objectIndex;

            std::cout << "Updated head and link to point to the same object" << std::endl;

            reinterpret_cast<DLLNode *>(ptr)->next = alloc->blocks[smallBlockMetada.data.usedMetaData.sizeClass];

            alloc->blocks[smallBlockMetada.data.usedMetaData.sizeClass] = reinterpret_cast<void*>((uintptr_t) block + cacheLineSize + sizeof(DLLNode) + sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint8_t));

            std::cout << "Updated DLL nodes and allocator block array" << std::endl;
            return;
        };

        if (smallBlockMetada.used == 0) {
            std::cout << "This small block is now completely free, can mark this as a free block once again and update superblock" << std::endl;

            metaData->freeCount++;
            metaData->used--;

            smallBlockMetada.data.next = metaData->head;

            std::cout << "This is where I have to use the DLL node, I'm removing something that could be part of the list";

            // TOOD

            if (metaData->freeCount == 1) {
                std::cout << "Super block now has a free block " << std::endl;

                metaData->dllNode.next = alloc->superBlocks;

                alloc->superBlocks = (void *) block;

                std::cout << "Updated super block array for allocator " << std::endl;
            };
        };
    };

};