;/*FROM http://wiki.osdev.org/Calling_Global_Constructors*/

;/* x86 crtn.s */
section .init
	;/* gcc will nicely put the contents of crtend.o's .init section here. */
	pop ebp
	ret

section .fini
;	/* gcc will nicely put the contents of crtend.o's .fini section here. */
	pop ebp
	ret
	