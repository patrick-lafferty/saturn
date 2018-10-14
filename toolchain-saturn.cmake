set(CMAKE_POLICY_DEFAULT_CMP0066 NEW)

SET(CMAKE_BUILD_TYPE Debug)

SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_VERSION 1)
SET(triple x86_64-saturn-pc)

SET(CMAKE_C_COMPILER clang)
SET(CMAKE_CXX_COMPILER clang++)
SET(CMAKE_ASM_COMPILER yasm)
SET(CMAKE_SYSROOT /home/pat/saturn/sysroot)
SET(CMAKE_FIND_ROOT_PATH /home/pat/saturn/sysroot)

SET(ONLY_CMAKE_FIND_ROOT_PATH TRUE)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

SET(CMAKE_LINKER "/home/pat/saturn-toolchain--x86_64/bin/x86_64-saturn-ld")
SET(CMAKE_C_LINK_EXECUTABLE "/home/pat/saturn-toolchain--x86_64/bin/x86_64-saturn-ld <CMAKE_C_LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
SET(CMAKE_CXX_LINK_EXECUTABLE "/home/pat/saturn-toolchain--x86_64/bin/x86_64-saturn-ld <CMAKE_CXX_LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")



SET(CMAKE_C_FLAGS "-isysroot /home/pat/saturn/sysroot -iwithsysroot /system/include -U__linux__ -U__GLIBC__ -U__unix_")
SET(CMAKE_CXX_FLAGS "-DBYTE_ORDER=LITTLE_ENDIAN -nostdinc++ -std=c++2a -iwithsysroot /system/include -D_LIBCPP_HAS_THREAD_API_EXTERNAL -U__linux__ -U__GLIBC__ -U__unix__ -I/home/pat/saturn-libc++/llvm/projects/libcxx/include")
SET(CMAKE_C_LINK_FLAGS " --sysroot=/home/pat/saturn/sysroot -L=/system/libraries")
SET(CMAKE_CXX_LINK_FLAGS " -nostdlib -nodefaultlibs  --sysroot=/home/pat/saturn/sysroot -L=/system/libraries")


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)