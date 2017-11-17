HigherStart = 0xD0000000;

ENTRY(_start)

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
		__kernel_memory_end = .;
	}
}