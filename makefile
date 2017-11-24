DEPENDENCYDIR := .d
DEPENDENCYFLAGS = -MT $@ -MMD -MP -MF $(DEPENDENCYDIR)/$*.Td

CROSS_COMPILER_PATH = ~/projects/saturn-cross-compiler/bin

AR = $(CROSS_COMPILER_PATH)/i686-elf-ar
AS = $(CROSS_COMPILER_PATH)/i686-elf-as
AS = yasm
ASFLAGS = -msyntax=intel -mmnemonic=intel -mnaked-reg -g
ASFLAGS = -felf32

CXX = clang++-5.0
MARCH = "--target=i686-pc-none-elf -march=i686"
WARNINGS = -Wall -Wextra 
CXXPATHS = -isysroot sysroot/ -iwithsysroot /system/include -I src -I src/libc/include -I src/libc++/include -I src/kernel -I src/kernel/arch/i386 -I .
	
FLAGS = -fno-omit-frame-pointer -ffreestanding -fno-exceptions -fno-rtti
CXXFLAGS = -O0 $(FLAGS) -g $(MARCH) $(DEPENDENCYFLAGS) $(WARNINGS) -std=c++1z  $(CXXPATHS) -masm=intel -Drestrict=__restrict

LD = $(CROSS_COMPILER_PATH)/i686-elf-ld
LDFLAGS = -L=system/lib --sysroot=sysroot/ -g

MKDIR = mkdir -p

ARCHDIR = src/kernel/arch/i386
SERVICESDIR = src/services

ARCH_OBJS = \
	$(ARCHDIR)/boot.o \
	$(ARCHDIR)/gdt/gdt_flush.o \
	$(ARCHDIR)/gdt/gdt.o \
	$(ARCHDIR)/idt/isr_stubs.o \
	$(ARCHDIR)/idt/loadIDT.o \
	$(ARCHDIR)/idt/idt.o \
	$(ARCHDIR)/cpu/cpuid.o \
	$(ARCHDIR)/cpu/cpuid_impl.o \
	$(ARCHDIR)/cpu/acpi.o \
	$(ARCHDIR)/cpu/initialize_sse.o \
	$(ARCHDIR)/cpu/pic.o \
	$(ARCHDIR)/cpu/apic.o \
	$(ARCHDIR)/cpu/change_process.o \
	$(ARCHDIR)/cpu/tss.o \
	$(ARCHDIR)/memory/scan_memory.o \
	$(ARCHDIR)/memory/physical_memory_manager.o \
	$(ARCHDIR)/memory/physical_memory_manager_pre_kernel.o \
	$(ARCHDIR)/memory/virtual_memory_manager.o \
	$(ARCHDIR)/memory/virtual_memory_manager_pre_kernel.o

KERNEL_OBJS = \
	$(ARCH_OBJS) \
	src/kernel/ipc.o \
	src/kernel/scheduler.o \
	src/kernel/services.o \
	src/kernel/pre_kernel.o \
	src/kernel/kernel.o

SERVICES_OBJS = \
	$(SERVICESDIR)/terminal/vga.o \
	$(SERVICESDIR)/terminal/terminal.o \
	$(SERVICESDIR)/splash/splash.o \

OBJS = \
	$(ARCHDIR)/crti.o \
	$(shell $(CC) $(CFLAGS) -m32 -print-file-name=crtbegin.o) \
	$(KERNEL_OBJS) \
	$(SERVICES_OBJS) \
	$(shell $(CC) $(CFLAGS) -m32 -print-file-name=crtend.o) \
	$(ARCHDIR)/crtn.o

OBJS_WITHOUT_CRT = \
	$(ARCHDIR)/crti.o \
	$(KERNEL_OBJS) \
	$(SERVICES_OBJS) \
	$(ARCHDIR)/crtn.o

DEPS = \
	$(patsubst %,$(DEPENDENCYDIR)/%.Td,$(basename $(OBJS_WITHOUT_CRT))) \
	$(patsubst %,$(DEPENDENCYDIR)/%.Td,$(basename $(LIBC_FREE_OBJS))) \
	$(patsubst %,$(DEPENDENCYDIR)/%.Td,$(basename $(TEST_LIBC_FREE_OBJS))) \
	$(patsubst %,$(DEPENDENCYDIR)/%.Td,$(basename $(LIBC++_FREE_OBJS))) \
	$(patsubst %,$(DEPENDENCYDIR)/%.Td,$(basename $(TEST_LIBC++_FREE_OBJS)))

include src/libc/make.config
include test/libc/make.config

include src/libc++/make.config
include test/libc++/make.config

$(shell mkdir -p $(dir $(DEPS)) >/dev/null)

LIBS = -lc_test_freestanding -lc++_test_freestanding -lc++_freestanding -lc_freestanding 

LINK_LIST = \
	$(LDFLAGS) \
	$(ARCHDIR)/crti.o \
	$(shell $(CC) $(CFLAGS) -m32 -print-file-name=crtbegin.o) \
	$(KERNEL_OBJS) \
	$(SERVICES_OBJS) \
	$(LIBS) \
	$(shell $(CC) $(CFLAGS) -m32 -print-file-name=crtend.o) \
	$(ARCHDIR)/crtn.o

all: sysroot deps saturn.bin

deps:
	$(MKDIR) .d/ -p

sysroot:
	$(MKDIR) sysroot/
	$(MKDIR) sysroot/system
	$(MKDIR) sysroot/system/boot
	$(MKDIR) sysroot/system/lib
	$(MKDIR) sysroot/system/include

saturn.bin: libc test_libc libc++ test_libc++ $(OBJS) $(ARCHDIR)/linker.ld
	$(LD) -T $(ARCHDIR)/linker.ld -o sysroot/system/boot/$@ $(LINK_LIST) $(LDFLAGS) 

libc: $(LIBC_FREE_OBJS)
	$(AR) rcs src/libc/libc_freestanding.a $(LIBC_FREE_OBJS)
	ranlib src/libc/libc_freestanding.a
	cp src/libc/libc_freestanding.a sysroot/system/lib
	cp -R --preserve=timestamps src/libc/include sysroot/system/

test_libc: $(TEST_LIBC_FREE_OBJS)
	$(AR) rcs test/libc/libc_test_freestanding.a $(TEST_LIBC_FREE_OBJS)
	ranlib test/libc/libc_test_freestanding.a
	cp test/libc/libc_test_freestanding.a sysroot/system/lib

libc++: $(LIBC++_FREE_OBJS)
	$(AR) rcs src/libc++/libc++_freestanding.a $(LIBC++_FREE_OBJS)
	ranlib src/libc++/libc++_freestanding.a
	cp src/libc++/libc++_freestanding.a sysroot/system/lib
	cp -R --preserve=timestamps src/libc++/include sysroot/system/

test_libc++: $(TEST_LIBC++_FREE_OBJS)
	$(AR) rcs test/libc++/libc++_test_freestanding.a $(TEST_LIBC++_FREE_OBJS)
	ranlib test/libc++/libc++_test_freestanding.a
	cp test/libc++/libc++_test_freestanding.a sysroot/system/lib

%.o: %.s
	$(AS) $< -o $@ $(ASFLAGS)

-include $(DEPS) 

%.o: %.cpp $(DEPENDENCYDIR)/%.d 
	$(CXX) -c $< -o $@ $(CXXFLAGS)

src/kernel/arch/i386/memory/physical_memory_manager_pre_kernel.o: src/kernel/arch/i386/memory/physical_memory_manager.cpp $(DEPENDENCYDIR)/src/kernel/arch/i386/memory/virtual_memory_manager.d
	$(CXX) -c $< -o $@ $(CXXFLAGS) -DTARGET_PREKERNEL

src/kernel/arch/i386/memory/virtual_memory_manager_pre_kernel.o: src/kernel/arch/i386/memory/virtual_memory_manager.cpp $(DEPENDENCYDIR)/src/kernel/arch/i386/memory/virtual_memory_manager.d
	$(CXX) -c $< -o $@ $(CXXFLAGS) -DTARGET_PREKERNEL

$(DEPENDENCYDIR)/%.d: ;

.PRECIOUS: $(DEPENDENCYDIR)/%.d

clean:
	$(RM) sysroot/ -rf
	$(RM) $(OBJS_WITHOUT_CRT) $(LIBC_FREE_OBJS)
	$(RM) src/libc/libc_freestanding.a
	$(RM) test/libc/libc_test_freestanding.a
	$(RM) .d/ -rf

.PHONY: all deps sysroot clean
