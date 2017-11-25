#include <new.h>
#include <stdlib.h>
#include <stdio.h>

/*
TODO: follow the c++ standard requirements for new, alignment etc

This is just a temporary implementation so I can work on bigger things
*/

void* operator new(std::size_t count) {
    return malloc(count);
}

void* operator new[](std::size_t count) {

    /*
    TODO: NOTE: from http://en.cppreference.com/w/cpp/language/new
    "Array allocation may supply unspecified overhead, which may vary from one 
    call to new to the next. The pointer returned by the new-expression will be 
    offset by that value from the pointer returned by the allocation function. 
    Many implementations use the array overhead to store the number of objects 
    in the array which is used by the delete[] expression to call the correct 
    number of destructors. In addition, if the new-expression is used to allocate 
    an array of char, unsigned char, or std::byte, it may request additional memory 
    from the allocation function if necessary to guarantee correct alignment of 
    objects of all types no larger than the requested array size, if one is later 
    placed into the allocated array. "
    */
    return malloc(count);
}

void* operator new(std::size_t count, std::align_val_t alignment) {
    return aligned_alloc(static_cast<std::size_t>(alignment), count);
}

void* operator new (std::size_t count, void* ptr) {
    return ptr;
}

void operator delete(void* ptr) {
    free(ptr);
}

void operator delete(void* ptr, std::align_val_t alignment) {
    free(ptr);
}