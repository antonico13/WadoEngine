#include <WinSDKVer.h>
#pragma comment(lib, "mincore")

#include <Windows.h>

#include <iostream>
#include <stdexcept>
#include <bitset>
#include <string>
#include <stdlib.h>

#include "WdMalloc.h"


const size_t BLOCK_EXPONENT = 21;
const size_t TWO_GIGABYTE_EXPONENT = 31;
const size_t BLOCK_SIZE = (size_t)1 << BLOCK_EXPONENT;
const size_t TWO_GIGABYTE = (size_t)1 << TWO_GIGABYTE_EXPONENT; // can fit 2^10 chunks in this allocation 
const size_t MEDIUM = 4096; // 4kb, should just make it page size maybe 

int main(int argc, char** argv) {
    Wado::Malloc::WdMalloc::mallocInit(TWO_GIGABYTE, 8);
    void *allocated[64];

    for (int i = 0; i < 64; i++) {
        allocated[i] = Wado::Malloc::WdMalloc::malloc((size_t)1 << 15);
    };

    Wado::Malloc::WdMalloc::free(allocated[0]);

    Wado::Malloc::WdMalloc::mallocShutdown();
    return 0;
};