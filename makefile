# Toolchain
CXX = clang++-5.0
AS = yasm
LD = x86_64-saturn-ld

# Global flags
ARCHDIR = src/kernel/arch/x86_64
GLOBAL_CXX_FLAGS =
GLOBAL_CXX_FLAGS += -MT $@ -MMD -MP -MF $(DEPENDENCYDIR)/$(strip $*).Td
GLOBAL_AS_FLAGS = -felf64

# Macros that create rules for each target
define ADD_OBJECT_RULES

src/$(1)/%.o: src/$(1)/%.cpp
	$(CXX) -c $$< -o $$@ $(GLOBAL_CXX_FLAGS) $($(1)_CXX_FLAGS)

src/$(1)/%.o: src/$(1)/%.s 
	$$(AS) $$< -o $$@ $(GLOBAL_AS_FLAGS) $$($(1)_AS_FLAGS)

endef

# Process all of the targets
include src/kernel/make.config
$(foreach target,$(TARGETS), $(eval $(call ADD_OBJECT_RULES,$(target))))

all: BINARIES

# Dependencies
DEPENDENCYDIR := .d
$(DEPENDENCYDIR)/%.d: ;
DEPENDENCIES = $(patsubst %,$(DEPENDENCYDIR)/%.Td,$(basename $(OBJECTS)))
.PRECIOUS: $(DEPENDENCYDIR)/%.d
-include $(DEPENDENCIES)

# Manage directory layout

MKDIR = mkdir -p

sysroot_directories:
	$(MKDIR) sysroot/
	$(MKDIR) sysroot/system
	$(MKDIR) sysroot/system/boot
	$(MKDIR) sysroot/system/libraries
	$(MKDIR) sysroot/system/include

%.Td: ;

dependency_directories: $(DEPENDENCIES)
	$(MKDIR) .d/ -p
	$(shell mkdir -p $(dir $(DEPENDENCIES)))

# End directory layout

clean:
	$(RM) sysroot/system/libraries -rf
	$(RM) sysroot/system/include -rf
	$(RM) sysroot/system/boot -rf
	$(RM) .d/ -rf
	$(RM) $(OBJECTS)

.PHONY: all sysroot_directories dependency_directories clean

#helpful rule that will display the value of something
#usage: make print-THING
#see https://stackoverflow.com/a/25817631/869619
print-%  : ; @echo $* = $($*)