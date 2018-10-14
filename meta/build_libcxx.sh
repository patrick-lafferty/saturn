#!/usr/bin/env bash

mkdir -p toolchain
cd toolchain

saturn="$(dirname $(pwd))"

if [ -d "$saturn/toolchain/libc++" ]; then
    exit 0;
fi

export SATURN_SYSROOT="$saturn/sysroot"
export SATURN_LIBCXX="$saturn/toolchain/llvm/projects/libcxx/include"
export SATURN_TOOLCHAIN_BIN="$saturn/toolchain/binutils/bin"

mkdir build_llvm -p
cd build_llvm

cmake -DCMAKE_TOOLCHAIN_FILE=../../meta/toolchain-saturn.cmake -C ../../meta/cache.cmake -G "Unix Makefiles" --debug-output --debug-trycompile ../llvm/ --trace
make cxx

cd ..
rm libc++ -rf
mkdir libc++/lib -p
cp build_llvm/lib/*.a libc++/lib
cp build_llvm/include/c++/v1 libc++/include -a

unset SATURN_SYSROOT
unset SATURN_LIBCXX
unset SATURN_TOOLCHAIN_BIN
