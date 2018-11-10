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

namespace Memory {

    /*
    AddressReservation is responsible for handing out memory addresses
    within its range, so the old-style vmm->allocatePages happens
    here essentially.
    */
    class AddressReservation {
    public:

        AddressReservation(uintptr_t start, uint64_t size);

        std::optional<uintptr_t> allocatePages(int count);

        friend bool operator==(const AddressReservation& left, const AddressReservation& right) {
            return left.startAddress == right.startAddress;
        }

        friend bool operator<(const AddressReservation& left, const AddressReservation& right) {
            return left.startAddress < right.startAddress;
        }

        AddressReservation split(uint64_t size);

        uint64_t getSize() const;
        void clearFreeFlag();
        bool isAvailable() const;

    private:

        uintptr_t startAddress;
        uint64_t size;
        bool isFree {true};
        uint64_t remainingPages; 
        uintptr_t nextPage;
    };

    inline bool operator>(const AddressReservation& left, const AddressReservation& right) {
        return operator<(right, left);
    }

    inline bool operator>=(const AddressReservation& left, const AddressReservation& right) {
        return !operator<(left, right);
    }

    class AddressSpace {
    public:

        AddressSpace();

        std::optional<AddressReservation> reserve(uint64_t size);
        void release(AddressReservation reservation);

    private:

        AddressSpace(const AddressSpace&) = delete;
        AddressSpace(const AddressSpace&&) = delete;

        struct Allocator {

            Allocator();

            int availableItems;
            int currentItems;
            uintptr_t currentPage {509ul << 39};

            template<class T, typename... Args>
            T* allocate(Args&&... args) {
                auto buffer = reinterpret_cast<T*>(currentPage);

                if (availableItems == 0) {
                    preparePage();
                    availableItems = 0x1000 / sizeof(T);
                    currentPage += 0x1000;
                }

                availableItems--;
                auto ptr = buffer + currentItems;
                currentItems++;

                return new (ptr) T(std::forward<Args>(args)...);
            }

            template<class T>
            void free(T*) {}

        private:

            void preparePage();
        };

        Allocator allocator;        
        AVLTree<AddressReservation, Allocator> space;
    };
}
