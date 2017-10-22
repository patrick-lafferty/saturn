CROSS_COMPILER_PATH = ~/projects/saturn-cross-compiler/bin

AR = $(CROSS_COMPILER_PATH)/i686-elf-ar
AS = $(CROSS_COMPILER_PATH)/i686-elf-as

CXX = clang++
MARCH = "--target=i686-pc-none-elf -march=i686"
WARNINGS = -Wall -Wextra --verbose
CXXFLAGS = $(MARCH) -ffreestanding -fno-exceptions -fno-rtti $(WARNINGS) -std=c++14 -isysroot sysroot/ -iwithsysroot /system/include

LD = $(CROSS_COMPILER_PATH)/i686-elf-ld
LDFLAGS = -L=system/lib --sysroot=sysroot/

MKDIR = mkdir -p

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

LIBS = -lc_freestanding

LINK_LIST = \
	$(LDFLAGS) \
	$(ARCHDIR)/crti.o \
	$(shell $(CC) $(CFLAGS) -m32 -print-file-name=crtbegin.o) \
	$(KERNEL_OBJS) \
	$(LIBS) \
	$(shell $(CC) $(CFLAGS) -m32 -print-file-name=crtend.o) \
	$(ARCHDIR)/crtn.o

include src/libc/make.config

all: sysroot saturn.bin

sysroot:
	$(MKDIR) sysroot/
	$(MKDIR) sysroot/system
	$(MKDIR) sysroot/system/boot
	$(MKDIR) sysroot/system/lib
	$(MKDIR) sysroot/system/include

saturn.bin: libc $(OBJS) $(ARCHDIR)/linker.ld
	$(LD) -T $(ARCHDIR)/linker.ld -o sysroot/system/boot/$@ $(LINK_LIST) $(LDFLAGS)

libc: $(LIBC_FREE_OBJS)
	$(AR) rcs src/libc/libc_freestanding.a $(LIBC_FREE_OBJS)
	ranlib src/libc/libc_freestanding.a
	cp src/libc/libc_freestanding.a sysroot/system/lib
	cp -R --preserve=timestamps src/libc/include sysroot/system/

%.o: %.s
	$(AS) $< -o $@

%.o: %.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

.PHONY: all sysroot