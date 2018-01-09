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
#include <stdlib.h>
#include <stdint.h>

typedef int (*comparison)(const void*, const void*);

void swap(void* a_, void* b_, size_t size) {
    auto a = static_cast<unsigned char*>(a_);
    auto b = static_cast<unsigned char*>(b_);
    
    for (auto i = 0u; i < size; i++) {
        auto temp = a[i];
        a[i] = b[i];
        b[i] = temp;
    }
}

uint32_t partition(unsigned char* array, size_t size, int start, int end, comparison compare) {
    auto pivot = array + (size * end);
    auto i = start;

    for (auto j = start; j < end; j++) {
        auto elementJ = array + (size * j);
        if (compare(elementJ, pivot) <= 0) {
            auto elementI = array + (size * i);
            swap(elementI, elementJ, size);
            i++;
        }
    }

    auto elementEnd = array + (size * end);
    auto elementI = array + (size * (i));

    swap(elementEnd, elementI, size);

    return i;
}

void quicksort(void* array, size_t size, int start, int end, comparison compare) {
    if (start < end) {
        auto partitionIndex = partition(static_cast<unsigned char*>(array), size, start, end, compare);
        quicksort(array, size, start, partitionIndex - 1, compare);
        quicksort(array, size, partitionIndex + 1, end, compare);
    }
}

void qsort(void* base, size_t count, size_t size, comparison compare) {

    quicksort(base, size, 0, count - 1, compare);
    
    /*
    TODO: implement this optimization
    
    const int threshold = 10;

    if (count < threshold) {
        insertionSort();
    }
    else {
        quickSort();
    }
    */


}