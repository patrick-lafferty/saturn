/*
Copyright (c) 2018, Patrick Lafferty
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "blockAllocator.h"
#include <misc/testing.h>
#include <memory/block_allocator.h>
#include <stdint.h>
#include <new>
#include <array>

namespace Test {

    using namespace Preflight;

    bool BlockAllocatorSuite::allocate_HandlesSimpleAllocations() {
        Memory::BlockAllocator<int> allocator(5);

        auto x1 = allocator.allocate();
        *x1 = 1;
        auto x2 = allocator.allocate();
        *x2 = 2;
        auto x3 = allocator.allocate();
        *x3 = 3;
        auto x4 = allocator.allocate();
        *x4 = 4;
        auto x5 = allocator.allocate();
        *x5 = 5;

        std::array<int, 5> result = {*x1, *x2, *x3, *x4, *x5};
        std::array<int, 5> expectedValues = {1, 2, 3, 4, 5};
        auto allValuesMatch = Assert::arraySame(result, expectedValues, "Allocated values were not 1,2,3,4,5");

        return Assert::all(allValuesMatch);
    }

    bool BlockAllocatorSuite::allocate_HandlesExcessiveAllocations() {
        Memory::BlockAllocator<int> allocator(5);
        int* allocations[100];

        std::array<int, 100> expected, actual;

        for (int i = 0; i < 100; i++) {
            allocations[i] = allocator.allocate();
            *allocations[i] = i;
            expected[i] = i;
        }

        //Note: separate loop incase successive memory writes overwrote previous values
        for (int i = 0; i < 100; i++) {
            actual[i] = *allocations[i];
        }

        auto allValuesMatch = Assert::arraySame(actual, expected, "Allocated values were not from 1 to 100");

        return Assert::all(allValuesMatch);
    }

    bool BlockAllocatorSuite::allocate_HandlesMultipleReservations() {

        Memory::BlockAllocator<int> allocator(5);
        int* allocations[100];

        std::array<int, 100> expected, actual;

        for (int i = 0; i < 100; i++) {
            allocations[i] = allocator.allocate();
            *allocations[i] = i;
            expected[i] = i;
        }

        //Note: separate loop incase successive memory writes overwrote previous values
        for (int i = 0; i < 100; i++) {
            actual[i] = *allocations[i];
        }

        auto allValuesMatch = Assert::arraySame(actual, expected, "Allocated values were not from 1 to 100");

        return Assert::all(allValuesMatch);
    }

    bool BlockAllocatorSuite::allocate_HandlesFreeList() {

        Memory::BlockAllocator<int> allocator(5);
        int* allocations[100];

        std::array<int, 100> expected, actual;

        for (int i = 0; i < 100; i++) {
            allocations[i] = allocator.allocate();
            *allocations[i] = i;
            expected[i] = i;
        }

        for (int i = 25; i < 75; i++) {
            allocator.free(allocations[i]);
        }

        for (int i = 25; i < 75; i++) {
            allocations[i] = allocator.allocate();
            *allocations[i] = i;
        }

        //Note: separate loop incase successive memory writes overwrote previous values
        for (int i = 0; i < 100; i++) {
            actual[i] = *allocations[i];
        }

        auto allValuesMatch = Assert::arraySame(actual, expected, "Allocated values were not from 1 to 100");

        return Assert::all(allValuesMatch);
    }

    bool BlockAllocatorSuite::run() {
        using namespace Preflight;

        return Preflight::runTests(
            test(allocate_HandlesSimpleAllocations, "Allocate handles simple allocations"),
            test(allocate_HandlesExcessiveAllocations, "Allocate handles excessive allocations"),
            test(allocate_HandlesMultipleReservations, "Allocate manages multiple AddressReservations"),
            test(allocate_HandlesFreeList, "Allocate prioritizes using freeList elements when possible")
        );
    }
}