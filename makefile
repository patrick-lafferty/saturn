DEPENDENCYDIR := .d
DEPENDENCYFLAGS = -MT $@ -MMD -MP -MF $(DEPENDENCYDIR)/$(strip $*).Td

CROSS_COMPILER_PATH = ~/projects/saturn-cross-compiler/bin/i686-saturn/bin

AR = $(CROSS_COMPILER_PATH)/ar
AS = $(CROSS_COMPILER_PATH)/as
AS = yasm
ASFLAGS = -msyntax=intel -mmnemonic=intel -mnaked-reg -g
ASFLAGS = -felf32

CXX = clang++-5.0
MARCH = "--target=i686-saturn -march=i686"
WARNINGS = -Wall -Wextra 
CXXPATHS = -isysroot sysroot/ -iwithsysroot /system/include -iwithsysroot /libraries/include/freetype2 -I src -I src/libc/include -I ../saturn-libc++/include/c++/v1 -I src/kernel -I src/kernel/arch/i386 -I .
	
FLAGS = -fno-omit-frame-pointer -ffreestanding -fno-exceptions -fno-rtti -fno-builtin -nostdinc
EXTRA_FLAGS = -D__ELF__ -D_LIBCPP_HAS_NO_THREADS -ferror-limit=0
CXXFLAGS = -O0 $(FLAGS) $(EXTRA_FLAGS) -g $(MARCH) $(DEPENDENCYFLAGS) $(WARNINGS) -std=c++1z  $(CXXPATHS) -masm=intel -Drestrict=__restrict

LD = $(CROSS_COMPILER_PATH)/ld
LDFLAGS = --sysroot=sysroot/ -L=system/libraries -L=libraries/lib -L=/home/pat/projects/saturn-libc++/lib -g

MKDIR = mkdir -p

include src/kernel/make.config
include src/services/make.config
include src/userland/make.config
include src/services/apollo/lib/make.config
include src/saturn/make.config

OBJS = \
	$(ARCHDIR)/crti.o \
	$(shell $(CC) $(CFLAGS) -m32 -print-file-name=crtbegin.o) \
	$(KERNEL_OBJS) \
	$(SERVICES_OBJS) \
	$(LIB_APOLLO_OBJS) \
	$(USERLAND_OBJS) \
	$(LIB_SATURN_OBJS) \
	$(shell $(CC) $(CFLAGS) -m32 -print-file-name=crtend.o) \
	$(ARCHDIR)/crtn.o

OBJS_WITHOUT_CRT = \
	$(ARCHDIR)/crti.o \
	$(KERNEL_OBJS) \
	$(SERVICES_OBJS) \
	$(LIB_APOLLO_OBJS) \
	$(USERLAND_OBJS) \
	$(LIB_SATURN_OBJS) \
	$(ARCHDIR)/crtn.o

DEPS = \
	$(patsubst %,$(DEPENDENCYDIR)/%.Td,$(basename $(OBJS_WITHOUT_CRT))) \
	$(patsubst %,$(DEPENDENCYDIR)/%.Td,$(basename $(LIBC_FREE_OBJS))) \
	$(patsubst %,$(DEPENDENCYDIR)/%.Td,$(basename $(TEST_LIBC_FREE_OBJS))) \
	$(patsubst %,$(DEPENDENCYDIR)/%.Td,$(basename $(LIBC_HOSTED_OBJS))) \
	$(patsubst %,$(DEPENDENCYDIR)/%.Td,$(basename $(USERLAND_OBJS))) \
	$(patsubst %,$(DEPENDENCYDIR)/%.Td,$(basename $(LIB_APOLLO_OBJS))) \
	$(patsubst %,$(DEPENDENCYDIR)/%.Td,$(basename $(LIB_SATURN_OBJS)))

include src/libc/make.config
include test/libc/make.config

$(shell mkdir -p $(dir $(DEPS)) >/dev/null)

LIBS = -luserland -lapollo -lfreetype -lsaturn -lc_test_freestanding -lc++ -lc++abi -lc_freestanding 

LINK_LIST = \
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
	$(MKDIR) sysroot/system/libraries
	$(MKDIR) sysroot/system/include

saturn.bin: libc_freestanding libc_hosted test_libc userland libapollo libsaturn $(OBJS) $(ARCHDIR)/linker.ld
	$(LD) -T $(ARCHDIR)/linker.ld -o sysroot/system/boot/$@ $(LDFLAGS) $(LINK_LIST)  

libc_freestanding: $(LIBC_FREE_OBJS)
	$(AR) rcs src/libc/libc_freestanding.a $(LIBC_FREE_OBJS)
	ranlib src/libc/libc_freestanding.a
	cp src/libc/libc_freestanding.a sysroot/system/libraries
	cp -R --preserve=timestamps src/libc/include sysroot/system/

libc_hosted: $(LIBC_HOSTED_OBJS) $(LIBC_HOSTED_NOT_LINKED_OBJS)
	$(AR) rcs src/libc/libc.a $(LIBC_HOSTED_OBJS)
	ranlib src/libc/libc.a
	cp src/libc/libc.a sysroot/system/libraries
	cp src/libc/hosted/crt1.o sysroot/system/libraries
	cp -R --preserve=timestamps src/libc/include sysroot/system/

test_libc: $(TEST_LIBC_FREE_OBJS)
	$(AR) rcs test/libc/libc_test_freestanding.a $(TEST_LIBC_FREE_OBJS)
	ranlib test/libc/libc_test_freestanding.a
	cp test/libc/libc_test_freestanding.a sysroot/system/libraries

userland: $(USERLAND_OBJS)
	$(AR) rcs src/userland/libuserland.a $(USERLAND_OBJS)
	ranlib src/userland/libuserland.a
	cp src/userland/libuserland.a sysroot/system/libraries

libapollo: $(LIB_APOLLO_OBJS)
	$(AR) rcs src/services/apollo/lib/libapollo.a $(LIB_APOLLO_OBJS)
	ranlib src/services/apollo/lib/libapollo.a
	cp src/services/apollo/lib/libapollo.a sysroot/system/libraries

libsaturn: $(LIB_SATURN_OBJS)
	$(AR) rcs src/saturn/libsaturn.a $(LIB_SATURN_OBJS)
	ranlib src/saturn/libsaturn.a
	cp src/saturn/libsaturn.a sysroot/system/libraries

%.o: %.s
	$(AS) $< -o $@ $(ASFLAGS)

%.o: %.cpp $(DEPENDENCYDIR)/%.d 
	$(CXX) -c $< -o $@ $(CXXFLAGS)

src/kernel/arch/i386/memory/physical_memory_manager_pre_kernel.o: src/kernel/arch/i386/memory/physical_memory_manager.cpp $(DEPENDENCYDIR)/src/kernel/arch/i386/memory/physical_memory_manager.d
	$(CXX) -c $< -o $@ $(CXXFLAGS) -DTARGET_PREKERNEL

src/kernel/arch/i386/memory/virtual_memory_manager_pre_kernel.o: src/kernel/arch/i386/memory/virtual_memory_manager.cpp $(DEPENDENCYDIR)/src/kernel/arch/i386/memory/virtual_memory_manager.d
	$(CXX) -c $< -o $@ $(CXXFLAGS) -DTARGET_PREKERNEL

$(DEPENDENCYDIR)/%.d: ;

.PRECIOUS: $(DEPENDENCYDIR)/%.d
-include $(DEPS) 

clean:
	$(RM) sysroot/system/libraries -rf
	$(RM) sysroot/system/include -rf
	$(RM) sysroot/system/boot -rf
	$(RM) $(OBJS_WITHOUT_CRT) $(LIBC_FREE_OBJS)
	$(RM) src/libc/libc_freestanding.a
	$(RM) test/libc/libc_test_freestanding.a
	$(RM) src/libc/libc.a
	$(RM) .d/ -rf

iso: 
	$(RM) sysroot/system/boot/saturn.iso
	mkdir sysroot/system/boot/grub -p
	cp src/grub.cfg sysroot/system/boot/grub
	grub-mkrescue -o sysroot/system/boot/saturn.iso sysroot/system

.PHONY: all deps sysroot clean
