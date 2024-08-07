#include "Windows.h"

#include "System.h"

#include <stdexcept>
#include <bitset>


#ifndef NDEBUG
#include <iostream>
#endif

namespace Wado::System {
    // TODO: no support for NUMA or more than 64 cores yet 
    const std::vector<WdCoreInfo> WdGetAvailableSystemCores() {
        std::vector<WdCoreInfo> coreInfos;
        HANDLE currentProcess = GetCurrentProcess();
        DWORD_PTR processAffinityMask;
        DWORD_PTR systemAffinityMask;
        BOOL res;

        res = GetProcessAffinityMask(currentProcess, &processAffinityMask, &systemAffinityMask);

        #ifndef NDEBUG
            std::cout << "Process affinity mask: " << (void *) (processAffinityMask) << std::endl;
        #endif

        if (res == FALSE) {
            throw std::runtime_error("Could not get system core information");
        };

        DWORD length;

        res = GetLogicalProcessorInformation(NULL, &length);

        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = static_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION>(HeapAlloc(GetProcessHeap(), 0, length));
        
        res = GetLogicalProcessorInformation(buffer, &length);

        if (res == FALSE) {
            throw std::runtime_error("Could not get logical core information");
        };

        DWORD seek = 0;
        length = length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        while (seek < length) {
            SYSTEM_LOGICAL_PROCESSOR_INFORMATION sysInfo = *(buffer + seek);
            #ifndef NDEBUG
            std::cout << "Found relationship type: " << sysInfo.Relationship << std::endl;
            #endif
            // TODO: Should I care about cache size and stuff here?
            if ((sysInfo.Relationship == RelationProcessorCore) && sysInfo.ProcessorCore.Flags) {
                // logical processors mentioned by the processor mask share a physical processor 
                // need to make sure all cores are usable 
                #ifndef NDEBUG
                    std::cout << "Processors: " << (void *) sysInfo.ProcessorMask << std::endl;
                    std::cout << "Flags: " << (int) sysInfo.ProcessorCore.Flags << std::endl;
                #endif
                if ( (sysInfo.ProcessorMask & processAffinityMask) == sysInfo.ProcessorMask) {
                    uint64_t firstCore = 0;
                    uint64_t secondCore = 0;
                    std::bitset<sizeof(ULONG_PTR)> processorMask = std::bitset<sizeof(ULONG_PTR)>(sysInfo.ProcessorMask);
                    for (size_t i = 0; i < sizeof(ULONG_PTR); ++i) {
                        if (processorMask[i]) {
                            firstCore = secondCore;
                            secondCore = i;
                        };
                    };

                    coreInfos.emplace_back(firstCore, secondCore);
                    coreInfos.emplace_back(secondCore, firstCore);
                };
            };
            ++seek;
        };

        HeapFree(GetProcessHeap, 0, buffer);

        // This should make use of RVO given that the return slot can hold a vector 
        return coreInfos;
    };
};