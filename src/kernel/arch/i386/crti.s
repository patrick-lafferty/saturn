;/*FROM http://wiki.osdev.org/Calling_Global_Constructors*/

section .init
global _init
_init:
	push ebp
	mov ebp, esp
;	

section .fini
global _fini
_fini:
	push ebp
	mov ebp, esp
;	
	