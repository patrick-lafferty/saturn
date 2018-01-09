%if 0

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

%endif
global setjmp
setjmp:
    mov ecx, [esp + 4]
    mov [ecx], ebx
    mov [ecx + 4], esi
    mov [ecx + 8], edi
    mov [ecx + 12], ebp
    lea edx, [esp + 4] 
    mov [ecx + 16], edx
    mov eax, [esp]
    mov [ecx + 20], eax ;eax
    xor eax, eax
    ret

global longjmp
longjmp:
    mov ecx, [esp + 4] 
    inc eax
    mov ebx, [ecx]
    mov esi, [ecx + 4]
    mov edi, [ecx + 8]
    mov ebp, [ecx  +12]
    mov esp, [ecx + 16]
    jmp [ecx + 20]
    ret
