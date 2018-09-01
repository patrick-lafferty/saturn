# Toolchain
CXX = clang++-5.0
AR = x86_64-saturn-ar
AS = yasm
LD = x86_64-saturn-ld

# Global flags
ARCHDIR = src/kernel/arch/x86_64
GLOBAL_CXX_FLAGS = -O0 -g -std=c++2a -Wall -Wextra -Wloop-analysis -Wpedantic -Wunreachable-code-aggressive
GLOBAL_CXX_FLAGS += -fno-omit-frame-pointer -ffreestanding -nostdinc -nostdinc++ -fno-rtti -fno-builtin -fno-exceptions
GLOBAL_CXX_FLAGS += -target x86_64-saturn-elf -D__ELF__ -D_LIBCPP_HAS_THREAD_API_EXTERNAL
# These flags are for libc++
TEMP_LIBCXX_PATH = ../saturn-libc++/llvm/projects/libcxx/include -I/usr/lib/llvm-5.0/lib/clang/5.0.2/include 
CXX_PATHS = -isysroot sysroot/ -iwithsysroot /system/include -I src -I src/libc/include -I src/kernel -I src/kernel/arch/x86_64 -I $(TEMP_LIBCXX_PATH)
GLOBAL_CXX_FLAGS += $(CXX_PATHS)
GLOBAL_CXX_FLAGS += -MT $@ -MMD -MP -MF .d/$(strip $@).Td
GLOBAL_AS_FLAGS = -felf64
LD_PATHS = --sysroot=sysroot/ -L=system/libraries
GLOBAL_LD_FLAGS = -g $(LD_PATHS)

# Macros that create rules for each target
define ADD_OBJECT_RULES

src/$(1)/%.o: src/$(1)/%.cpp 
	$(CXX) -c $$< -o $$@ $$(GLOBAL_CXX_FLAGS) $($(1)_CXX_FLAGS)

src/$(1)/%.o: src/$(1)/%.s 
	$$(AS) $$< -o $$@ $$(GLOBAL_AS_FLAGS) $$($(1)_AS_FLAGS)

endef

# Process all of the targets
include src/loader/make.config
include src/kernel/make.config
include src/libc/make.config
$(foreach target,$(TARGETS), $(eval $(call ADD_OBJECT_RULES,$(target))))

.DEFAULT_GOAL := all
all: sysroot_directories $(BINARIES)
	cp src/grub.cfg sysroot/system/boot/grub/grub.cfg
	$(RM) sysroot/system/boot/saturn.iso
	grub-mkrescue -o sysroot/system/boot/saturn.iso sysroot/system 

# Dependency management
DEPENDENCIES = $(patsubst %,.d/%.o.Td,$(basename $(OBJECTS)))
%.Td: ;

# Manage directory layout

MKDIR = mkdir -p

sysroot_directories:
	$(MKDIR) sysroot/ -p
	$(MKDIR) sysroot/system -p
	$(MKDIR) sysroot/system/boot -p
	$(MKDIR) sysroot/system/boot/grub -p
	$(MKDIR) sysroot/system/libraries -p
	$(MKDIR) sysroot/system/include -p

dependency_directories: 
	$(MKDIR) .d/ -p
	$(shell mkdir -p $(dir $(DEPENDENCIES)))

# End directory layout

clean:
	$(RM) sysroot/system/libraries -rf
	$(RM) sysroot/system/include -rf
	$(RM) sysroot/system/boot/*.*
	$(RM) .d/ -rf
	$(RM) $(OBJECTS)

# Runners
QEMU_ARGS = -cpu max -smp 2 -m 512M -s -no-reboot -no-shutdown
QEMU_ARGS += -cdrom sysroot/system/boot/saturn.iso -serial file:saturn.log

UNAME = $(shell uname -r)

ifneq (,$(findstring Microsoft,$(UNAME)))
RUNNER_EXTENSION = .exe
endif

QEMU = qemu-system-x86_64$(RUNNER_EXTENSION)
QEMU32 = qemu-system-i386$(RUNNER_EXTENSION)

qemu:
	$(QEMU) $(QEMU_ARGS)&

qemu32:
	$(QEMU32) $(QEMU_ARGS)&

bochs:
	./meta/bochs-runner.sh

virtualbox:
	./meta/vbox-runner.sh

.PHONY: all sysroot_directories dependency_directories clean qemu qemu32 bochs virtualbox
-include $(DEPENDENCIES)

#helpful rule that will display the value of something
#usage: make print-THING
#see https://stackoverflow.com/a/25817631/869619
print-%  : ; @echo $* = $($*)