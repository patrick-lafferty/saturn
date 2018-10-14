#!/usr/bin/env bash

mkdir -p toolchain
cd toolchain

saturn="$(dirname $(pwd))"

if [ -d "$saturn/toolchain/llvm" ]; then
    exit 0;
fi

svn co http://llvm.org/svn/llvm-project/llvm/trunk llvm
cd llvm/projects
svn co http://llvm.org/svn/llvm-project/libcxx/trunk libcxx
svn co http://llvm.org/svn/llvm-project/libcxxabi/trunk libcxxabi

cd ..