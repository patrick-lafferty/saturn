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
#pragma once

#include <optional>
#include <misc/avl_tree.h>
#include "addresses.h"

namespace Memory {

    inline const uint64_t PDPSize = 0x8000000000ul;

    /*
    AddressReservation is responsible for handing out memory addresses
    within its range, so the old-style vmm->allocatePages happens
    here essentially.
    */
    class AddressReservation {
    public:

        AddressReservation(VirtualAddress start, uint64_t size);

        std::optional<VirtualAddress> allocatePages(uint64_t count);

        friend bool operator==(const AddressReservation& left, const AddressReservation& right) {
            return left.start.address == right.start.address;
        }

        friend bool operator<(const AddressReservation& left, const AddressReservation& right) {
            return left.start.address < right.start.address;
        }

        AddressReservation split(uint64_t size);

        uint64_t getSize() const;
        void clearFreeFlag();
        bool isAvailable() const;

    private:

        VirtualAddress start;
        uint64_t size;
        bool isFree {true};
        uint64_t remainingPages; 
        VirtualAddress nextPage;
    };

    inline bool operator>(const AddressReservation& left, const AddressReservation& right) {
        return operator<(right, left);
    }

    inline bool operator>=(const AddressReservation& left, const AddressReservation& right) {
        return !operator<(left, right);
    }

    class AddressSpace {
    public:

        enum class Domain {
            Default,
            KernelStacks,
            Last
        };

        AddressSpace();
        AddressSpace(VirtualAddress start, uint64_t size, Domain domain);

        std::optional<AddressReservation> reserve(uint64_t size);
        void release(AddressReservation reservation);

    private:

        AddressSpace(const AddressSpace&) = delete;
        AddressSpace(const AddressSpace&&) = delete;

        struct Allocator {

            Allocator(Domain domain);

            int availableItems {0};
            int currentItems {0};
            VirtualAddress currentPage {(0xFFFFul << 48) + (509ul << 39)};
            VirtualAddress currentBuffer {(0xFFFFul << 48) + (509ul << 39)};

            template<class T, typename... Args>
            T* allocate(Args&&... args) {
                
                struct Allocation {
                    uintptr_t nextFree;
                    T value;
                };

                if (freeList != 0) {
                    auto allocation = reinterpret_cast<Allocation*>(freeList);
                    freeList = allocation->nextFree;
                    allocation->nextFree = 0;
                    allocation->value = T {std::forward<Args>(args)...};
                    return &allocation->value;
                }

                if (availableItems == 0) {
                    preparePage(sizeof(Allocation));
                    currentBuffer = currentPage;
                    currentPage.address += 0x1000;
                }

                auto buffer = reinterpret_cast<Allocation*>(currentBuffer.address);

                availableItems--;
                auto ptr = buffer + currentItems;
                currentItems++;

                auto item = new (ptr) Allocation {0, T {std::forward<Args>(args)...}};

                return &item->value;
            }

            template<class T>
            void free(T* item) {
                struct Allocation {
                    uintptr_t nextFree;
                    T value;
                };

                if (freeList == 0) {
                    auto address = reinterpret_cast<uintptr_t>(item) - sizeof(uintptr_t);
                    auto ptr = reinterpret_cast<Allocation*>(address);
                    ptr->nextFree = 0;
                    freeList = address;
                } 
                else {
                    auto address = reinterpret_cast<uintptr_t>(item) - sizeof(uintptr_t);
                    auto ptr = reinterpret_cast<Allocation*>(address);
                    ptr->nextFree = freeList;
                    freeList = address;
                }
            }

        private:

            void preparePage(size_t itemSize);

            uintptr_t freeList {0};
        };

        Allocator allocator;        
        AVLTree<AddressReservation, Allocator> space;
    };
}
