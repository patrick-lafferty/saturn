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

#include "linked_list.h"
#include <misc/testing.h>
#include <misc/linked_list.h>
#include <stdint.h>
#include <new>
#include <array>
#include <memory/block_allocator.h>

namespace Test {

    using namespace Preflight;
    using namespace Memory;

    bool LinkedListSuite::add_HandlesEmptyList() {
        LinkedList<int, BlockAllocator> list;

        list.add(42);

        auto theAnswerToEverything = Assert::isEqual(list.getHead()->value, 42, "LinkedList doesn't have the answer to the Ultimate Question of Life, the Universe, and Everything");

        return Assert::all(theAnswerToEverything);
    }

    bool LinkedListSuite::add_HandlesNonEmptyList() {
        LinkedList<int, BlockAllocator> list;

        list.add(2);
        list.add(3);
        list.add(5);
        list.add(7);
        list.add(11);

        std::array<int, 5> expected = {11, 7, 5, 3, 2};
        std::array<int, 5> result;

        auto head = list.getHead();

        for (int i = 0; i < 5; i++) {
            result[i] = head->value;
            head = head->next;
        }

        auto primesBePrimed = Assert::arraySame(result, expected, "LinkedList doesn't handle multiple adds");

        return Assert::all(primesBePrimed);
    }

    bool LinkedListSuite::reverse_Works() {
        LinkedList<int, BlockAllocator> list;

        list.add(2);
        list.add(3);
        list.add(5);
        list.add(7);
        list.add(11);

        std::array<int, 5> expected = {2, 3, 5, 7, 11};
        std::array<int, 5> result;

        list.reverse();

        auto head = list.getHead();

        for (int i = 0; i < 5; i++) {
            result[i] = head->value;
            head = head->next;
        }

        auto primesBePrimed = Assert::arraySame(result, expected, "LinkedList reverse doesn't work as expected");

        return Assert::all(primesBePrimed);
    }

    bool LinkedListSuite::run() {
        using namespace Preflight;

        return Preflight::runTests(
            test(add_HandlesEmptyList, "Add handles empty lists"),
            test(add_HandlesNonEmptyList, "Add handles non-empty lists"),
            test(reverse_Works, "Reverse works")
        );
    }
}