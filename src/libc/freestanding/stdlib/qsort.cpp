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