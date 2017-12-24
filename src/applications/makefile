CROSS_COMPILER_PATH = ~/projects/saturn-cross-compiler/bin/i686-saturn/bin

AR = $(CROSS_COMPILER_PATH)/ar

CXX = clang++-5.0
MARCH = "--target=i686-saturn -march=i686"
WARNINGS = -Wall -Wextra 
CXXPATHS = -isysroot ../../sysroot/ -iwithsysroot /system/include -iwithsysroot
	
FLAGS = -fno-omit-frame-pointer -ffreestanding -fno-exceptions -fno-rtti -fno-builtin -nostdinc 
CXXFLAGS = -O0 $(FLAGS) -g $(MARCH) $(DEPENDENCYFLAGS) $(WARNINGS) -std=c++1z  $(CXXPATHS) -masm=intel -Drestrict=__restrict

LD = $(CROSS_COMPILER_PATH)/ld
LDFLAGS = --sysroot=../../sysroot/ -L=system/lib -L=libraries/lib -g

OBJS = test/main.o

LIBS = -lc_freestanding

LINK_LIST = \
	$(OBJS) \
	$(LIBS)

all: test.bin 

test.bin: $(OBJS) 
	$(LD) -o test/$@ $(LDFLAGS) $(LINK_LIST) 

%.o: %.cpp 
	$(CXX) -c $< -o $@ $(CXXFLAGS)
