/*
Copyright (c) 2017, Patrick Lafferty
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

#include <stdint.h>

namespace Kernel {

    template<typename T> class LinkedList {
        public:
            
            void insertBefore(T* item, T* before) {
                if (head == nullptr) {
                    head = item;
                }
                else {
                    item->nextTask = before;

                    if (before->previousTask != nullptr) {
                        item->previousTask = before->previousTask;
                        before->previousTask->nextTask = item;
                    }
                    else {
                        head = item;
                        item->previousTask = nullptr;
                    }

                    before->previousTask = item;
                }
            }

            void insertAfter(T* item, T* after) {
                after->nextTask = item;
                item->previousTask = after;
            }

            void append(T* item) {
                if (head == nullptr) {
                    head = item;
                    item->previousTask = nullptr;
                }
                else {
                    auto current = head;

                    while (current->nextTask != nullptr) {
                        current = current->nextTask;
                    }

                    current->nextTask = item;
                    item->previousTask = current;
                }

                item->nextTask = nullptr;
            }

            void remove(T* item) {
                auto previous = item->previousTask;
                auto next = item->nextTask;

                if (previous != nullptr) {
                    previous->nextTask = next;
                }
                else {
                    head = next;
                }

                if (next != nullptr) {
                    next->previousTask = previous;
                }

                item->previousTask = nullptr;
                item->nextTask = nullptr;
            }

            T* getHead() {
                return head;
            }

            bool isEmpty() {
                return head == nullptr;
            }

            uint32_t* getLock() {
                return &lock;
            }

        private:

        T* head {nullptr};
        uint32_t lock {0};
    };
}
