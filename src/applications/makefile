CROSS_COMPILER_PATH = ~/projects/saturn-cross-compiler/bin/i686-saturn/bin

AR = $(CROSS_COMPILER_PATH)/ar

CXX = clang++-5.0
MARCH = "--target=i686-saturn -march=i686"
WARNINGS = -Wall -Wextra 
CXXPATHS = -isysroot ../../sysroot/ -iwithsysroot /system/include -iwithsysroot
	
FLAGS = -fno-omit-frame-pointer -fno-exceptions -fno-rtti -fno-builtin -nostdinc 
CXXFLAGS = -O0 $(FLAGS) -g $(MARCH) $(DEPENDENCYFLAGS) $(WARNINGS) -std=c++1z  $(CXXPATHS) -masm=intel -Drestrict=__restrict

LD = $(CROSS_COMPILER_PATH)/ld
LDFLAGS = --sysroot=../../sysroot/ -L=system/libraries -L=libraries/lib -g

OBJS = \
	../../sysroot/system/libraries/crt1.o \
	../kernel/arch/i386/crti.o \
	$(shell $(CC) $(CFLAGS) -m32 -print-file-name=crtbegin.o) \
	test/main.o \
	$(shell $(CC) $(CFLAGS) -m32 -print-file-name=crtend.o) \
	../kernel/arch/i386/crtn.o \

LIBS = -lc

LINK_LIST = \
	$(OBJS) \
	$(LIBS)

all: test.bin 

test.bin: $(OBJS) 
	$(LD) -o test/$@ $(LDFLAGS) $(LINK_LIST) 

%.o: %.cpp 
	$(CXX) -c $< -o $@ $(CXXFLAGS)
