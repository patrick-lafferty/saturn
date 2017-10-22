CROSS_COMPILER_PATH = ~/projects/saturn-cross-compiler/bin

AS = $(CROSS_COMPILER_PATH)/i686-elf-as

CXX = clang++
MARCH = "--target=i686-pc-none-elf -march=i686"
WARNINGS = -Wall -Wextra
CXXFLAGS = $(MARCH) -ffreestanding -fno-exceptions -fno-rtti $(WARNINGS) -std=c++14

LD = $(CROSS_COMPILER_PATH)/i686-elf-ld

ARCHDIR = src/kernel/arch/i386

ARCH_OBJS = \
	$(ARCHDIR)/boot.o

KERNEL_OBJS = \
	$(ARCH_OBJS) \
	src/kernel/vga.o \
	src/kernel/terminal.o \
	src/kernel/kernel.o

OBJS = \
	$(ARCHDIR)/crti.o \
	$(shell $(CC) $(CFLAGS) -m32 -print-file-name=crtbegin.o) \
	$(KERNEL_OBJS) \
	$(shell $(CC) $(CFLAGS) -m32 -print-file-name=crtend.o) \
	$(ARCHDIR)/crtn.o

LINK_LIST = \
	$(LDFLAGS) \
	$(ARCHDIR)/crti.o \
	$(shell $(CC) $(CFLAGS) -m32 -print-file-name=crtbegin.o) \
	$(KERNEL_OBJS) \
	$(LIBS) \
	$(shell $(CC) $(CFLAGS) -m32 -print-file-name=crtend.o) \
	$(ARCHDIR)/crtn.o

all: saturn.bin

saturn.bin: $(OBJS) $(ARCHDIR)/linker.ld
	
	$(LD) -T $(ARCHDIR)/linker.ld -o $@ $(LINK_LIST) 


%.o: %.s
	$(AS) $< -o $@

%.o: %.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

PHONY: all