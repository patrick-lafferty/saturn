
global mems
mems:

mov edi, eax
mov esi, ecx
mov ecx, edx
mov edx, esi
add edx, ecx

start:

movntdqa xmm0, [esi]
movntdqa xmm1, [esi + 16]
movntdqa xmm2, [esi + 32]
movntdqa xmm3, [esi + 48]

movdqa [edi], xmm0
movdqa [edi + 16], xmm1
movdqa [edi + 32], xmm2
movdqa [edi + 48], xmm3

add esi, 040h
add edi, 040h
cmp esi, edx
jne start
ret