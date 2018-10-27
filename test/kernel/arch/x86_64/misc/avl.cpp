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

#include "avl.h"
#include <misc/testing.h>
#include <misc/avl_tree.h>
#include <stdint.h>
#include <new>
#include <array>

namespace Test {

    using namespace Preflight;


    /*struct Address {
        uint64_t start;
        uint64_t size;
    };*/

    template<unsigned int Size>
    class SimpleAllocator {
    public:

        SimpleAllocator() 
            : next {&staticBuffer[0]} {}

        template<class T>
        T* allocate() {
            auto ptr = next;
            next += sizeof(T);
            return new (ptr) T;
        }

    private:

        uint8_t staticBuffer[Size];
        uint8_t* next;
    };

    bool AVLTreeSuite::insertHandlesEmptyTree() {
        SimpleAllocator<480> allocator;
        AVLTree<int, SimpleAllocator<480>> tree {allocator};

        tree.insert(42);

        std::array<int, 1> result;
        tree.traverseInOrder(result);

        auto numberOfNodes = Assert::isEqual(tree.getNumberOfNodes(), 1, "Tree doesn't have 1 node");
        std::array<int, 1> expectedTraversal = {42};
        auto inorder = Assert::arraySame(result, expectedTraversal, "In-order traversal isn't [42]");

        return Assert::all(numberOfNodes, inorder);
    }

    bool AVLTreeSuite::insertHandlesSimpleTree() {
        SimpleAllocator<480> allocator;
        AVLTree<int, SimpleAllocator<480>> tree {allocator};

        tree.insert(42);
        tree.insert(8);
        tree.insert(69);

        std::array<int, 3> result;
        tree.traverseInOrder(result);

        auto numberOfNodes = Assert::isEqual(tree.getNumberOfNodes(), 3, "Tree doesn't have 3 nodes");
        std::array<int, 3> expectedTraversal = {8, 42, 69};
        auto inorder = Assert::arraySame(result, expectedTraversal, "In-order traversal isn't [8, 42, 69]");

        return Assert::all(numberOfNodes, inorder);
    }

    bool AVLTreeSuite::insertBalancesTree() {

        struct TestData {
            std::array<int, 8> values;
            std::array<int, 8> expectedTraversal;
        };

        auto test = [](TestData data) { 
            SimpleAllocator<480> allocator;
            AVLTree<int, SimpleAllocator<480>> tree {allocator};

            for (auto value : data.values) {
                tree.insert(value);
            }

            std::array<int, 8> result;
            tree.traverseInOrder(result);

            auto height = Assert::isEqual(tree.getHeight(), 4, "Tree height isn't 4");
            auto inorder = Assert::arraySame(result, data.expectedTraversal, "In-order traversal didn't match");

            return Assert::all(height, inorder);
        };

        return runCases(test, 
            TestData {{41, 20, 50, 29, 65, 11, 26, 23}, {11, 20, 23, 26, 29, 41, 50, 65}},
            TestData {{16, 13, 5, 10, 15, 11, 6, 4}, {4, 5, 6, 10, 11, 13, 15, 16}});
    }

    bool AVLTreeSuite::findAtLeast_HandlesEmptyTree() {
        SimpleAllocator<480> allocator;
        AVLTree<int, SimpleAllocator<480>> tree {allocator};

        auto result = tree.findAtLeast(5);
        auto isEmpty = Assert::isTrue(!result.has_value(), "result shouldn't have a value");

        return Assert::all(isEmpty);
    }

    bool AVLTreeSuite::findAtLeast_HandlesSingleTreeEqual() {
        SimpleAllocator<480> allocator;
        AVLTree<int, SimpleAllocator<480>> tree {allocator};

        tree.insert(5);
        auto result = tree.findAtLeast(5);
        auto hasValue = Assert::isTrue(result.has_value(), "result should have a value");
        auto value = Assert::isEqual(result, 5, "result should have a value");

        return Assert::all(hasValue, value);
    }

    bool AVLTreeSuite::findAtLeast_HandlesSingleTreeGreater() {
        SimpleAllocator<480> allocator;
        AVLTree<int, SimpleAllocator<480>> tree {allocator};

        tree.insert(10);
        auto result = tree.findAtLeast(5);
        auto hasValue = Assert::isTrue(result.has_value(), "result should have a value");
        auto value = Assert::isEqual(result, 10, "result should have a value");

        return Assert::all(hasValue, value);
    }

    bool AVLTreeSuite::findAtLeast_HandlesSingleTreeLess() {
        SimpleAllocator<480> allocator;
        AVLTree<int, SimpleAllocator<480>> tree {allocator};

        tree.insert(1);
        auto result = tree.findAtLeast(5);
        auto hasValue = Assert::isFalse(result.has_value(), "result shouldn't have a value");

        return Assert::all(hasValue);
    }

    bool AVLTreeSuite::findAtLeast_HandlesSimpleTree() {
         struct TestData {
            std::array<int, 8> values;
            int search;
            int result;
        };

        auto test = [](TestData data) { 
            SimpleAllocator<480> allocator;
            AVLTree<int, SimpleAllocator<480>> tree {allocator};

            for (auto value : data.values) {
                tree.insert(value);
            }

            auto result = tree.findAtLeast(data.search);
            auto equal = Assert::isEqual(result, data.result, "Couldn't find proper successor");

            return Assert::all(equal);
        };

        return runCases(test, 
            TestData {{41, 20, 50, 29, 65, 11, 26, 23}, 23, 23},
            TestData {{41, 20, 50, 29, 65, 11, 26, 23}, 24, 26},
            TestData {{41, 20, 50, 29, 65, 11, 26, 23}, 65, 65},
            TestData {{41, 20, 50, 29, 65, 11, 26, 23}, 10, 11},
            TestData {{41, 20, 50, 29, 65, 11, 26, 23}, 11, 11});
    }

    bool AVLTreeSuite::run() {
        using namespace Preflight;

        return Preflight::runTests(
            test(insertHandlesEmptyTree, "Insert handles empty trees"),
            test(insertHandlesSimpleTree, "Insert handles simple trees"),
            test(insertBalancesTree, "Insert balances tree when needed"),
            test(findAtLeast_HandlesEmptyTree, "FindAtLeast handles empty trees"),
            test(findAtLeast_HandlesSingleTreeEqual, "FindAtLeast handles single trees with equal root"),
            test(findAtLeast_HandlesSingleTreeGreater, "FindAtLeast handles single trees with greater root"),
            test(findAtLeast_HandlesSingleTreeLess, "FindAtLeast handles single trees with less root"),
            test(findAtLeast_HandlesSimpleTree, "FindAtLeast handles simple trees")
        );
    }
}