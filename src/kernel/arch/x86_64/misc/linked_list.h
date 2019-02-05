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

template<class T, template<class> class Allocator>
class LinkedList {

public:

    struct Node {
        Node* next {nullptr};
        T value;
    };

    ~LinkedList() {
        allocator.releaseMemory();
    }

    const Node* getHead() const {
        return head;
    }

    /*
    Prepends the value to the head of the list
    */
    void add(T value) {
        if (head == nullptr) {
            head = allocator.allocate();
            head->value = value;
        }
        else {
            auto node = allocator.allocate();
            node->value = value;
            node->next = head;
            head = node;
        }

        size++;
    }

    /*
    Reverses the list in-place
    */
    void reverse() {
        Node* reversed = nullptr;

        while (head != nullptr) {
            auto next = head->next;
            head->next = reversed;
            reversed = head;
            head = next;
        }

        head = reversed;
    }

    int getSize() const { return size; }

private:

    Node* head {nullptr};
    Allocator<Node> allocator;
    int size {0};
};