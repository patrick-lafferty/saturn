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
#include <new>
#include <stdlib.h>
#include <stdio.h>

extern "C" void __cxa_pure_virtual() {

}

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

void operator delete[](void* ptr) {
    free(ptr);
}

void operator delete(void* ptr, std::align_val_t alignment) {
    free(ptr);
}