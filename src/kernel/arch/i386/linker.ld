/*
TODO - rewrite, copied from osdev barebones
*/

HigherStart = 0xD0000000;

/* The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */
ENTRY(_start)

/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS
{
	/* Begin putting sections at 1 MiB, a conventional place for kernels to be
	   loaded at by the bootloader. */
	. = 1M;

	.setup ALIGN(4K):
	{
		__kernel_memory_start = .;
		*(.multiboot)
		*(.setup)
		src/kernel/arch/i386/memory/physical_memory_manager_pre_kernel.o (.text)
		src/kernel/arch/i386/memory/virtual_memory_manager_pre_kernel.o (.text)
		src/kernel/pre_kernel.o (.text)
	}

	.rodatas ALIGN(4K): {
		src/kernel/arch/i386/memory/physical_memory_manager_pre_kernel.o (.rodata)
		src/kernel/arch/i386/memory/virtual_memory_manager_pre_kernel.o (.rodata)
		src/kernel/pre_kernel.o (.rodata)
	}

	.datas ALIGN(4K): {
		src/kernel/arch/i386/memory/physical_memory_manager_pre_kernel.o (.data)
		src/kernel/arch/i386/memory/virtual_memory_manager_pre_kernel.o (.data)
		src/kernel/pre_kernel.o (.data)
	}

	.bsss ALIGN(4K): {
		src/kernel/arch/i386/memory/physical_memory_manager_pre_kernel.o (.bss, COMMON)
		src/kernel/arch/i386/memory/virtual_memory_manager_pre_kernel.o (.bss, COMMON)
		src/kernel/pre_kernel.o (.bss, COMMON)
	}


	. += HigherStart;
	/*. = 1M + HigherStart;*/

	/* First put the multiboot header, as it is required to be put very early
	   early in the image or the bootloader won't recognize the file format.
	   Next we'll put the .text section. */
	.text ALIGN(4K) : AT(ADDR(.text) - HigherStart)
	{
		/*__kernel_memory_start = .; - HigherStart;*/
		*(.text)
		src/kernel/arch/i386/memory/physical_memory_manager.o (.text)
		src/kernel/arch/i386/memory/virtual_memory_manager.o (.text)
	}

	/* Read-only data. */
	.rodata ALIGN(4K) : AT(ADDR(.rodata) - HigherStart)
	{
		*(.rodata)
		src/kernel/arch/i386/memory/physical_memory_manager.o (.rodata)
		src/kernel/arch/i386/memory/virtual_memory_manager.o (.rodata)
	}

	/* Read-write data (initialized) */
	.data ALIGN(4K) : AT(ADDR(.data) - HigherStart)
	{
		*(.data)
		src/kernel/arch/i386/memory/physical_memory_manager.o (.data)
		src/kernel/arch/i386/memory/virtual_memory_manager.o (.data)
	}

	/* Read-write data (uninitialized) and stack */
	.bss ALIGN(4K) : AT(ADDR(.bss) - HigherStart)
	{
		*(COMMON)
		*(.bss)
		src/kernel/arch/i386/memory/physical_memory_manager.o (.bss, COMMON)
		src/kernel/arch/i386/memory/virtual_memory_manager.o (.bss, COMMON)
		__kernel_memory_end = .;/* - HigherStart;*/
	}

	/* The compiler may produce other sections, by default it will put them in
	   a segment with the same name. Simply add stuff here as needed. */
}