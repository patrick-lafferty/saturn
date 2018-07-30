# Toolchain
CXX = clang++-5.0
AS = yasm
LD = x86_64-saturn-ld

# Global flags
ARCHDIR = src/kernel/arch/x86_64
GLOBAL_CXX_FLAGS = -O0 -g -std=c++2a -Wall -Wextra -Wloop-analysis -Wpedantic -Wunreachable-code-aggressive
GLOBAL_CXX_FLAGS += -fno-omit-frame-pointer -ffreestanding -nostdinc -fno-rtti -fno-builtin -fno-exceptions
# These flags are for libc++
#GLOBAL_CXX_FLAGS += -D__ELF__
#GLOBAL_CXX_FLAGS += -isysroot sysroot/ -iwithsysroot /system/include -iwithsysroot /libraries/include/freetype2 -I src -I src/libc/include -I ../saturn-libc++/include/c++/v1 .
GLOBAL_CXX_FLAGS += -MT $@ -MMD -MP -MF .d/$(strip $@).Td
GLOBAL_AS_FLAGS = -felf64
GLOBAL_LD_FLAGS = -g

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
$(foreach target,$(TARGETS), $(eval $(call ADD_OBJECT_RULES,$(target))))

.DEFAULT_GOAL := all
all: $(BINARIES)
	cp src/grub.cfg sysroot/system/boot/grub/grub.cfg
	$(RM) sysroot/system/boot/saturn.iso
	grub-mkrescue -o sysroot/system/boot/saturn.iso sysroot/system 

# Dependency management
DEPENDENCIES = $(patsubst %,.d/%.o.Td,$(basename $(OBJECTS)))
%.Td: ;

# Manage directory layout

MKDIR = mkdir -p

sysroot_directories:
	$(MKDIR) sysroot/
	$(MKDIR) sysroot/system
	$(MKDIR) sysroot/system/boot
	$(MKDIR) sysroot/system/boot/grub
	$(MKDIR) sysroot/system/libraries
	$(MKDIR) sysroot/system/include

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