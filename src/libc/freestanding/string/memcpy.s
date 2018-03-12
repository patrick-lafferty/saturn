global memcpy
memcpy:

push ebp
mov ebp, esp

mov edi, [ebp + 8]
mov eax, edi
mov esi, [ebp + 12]
mov ecx, [ebp + 16]

rep movsb

mov esp, ebp
pop ebp

ret