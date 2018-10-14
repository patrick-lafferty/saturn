#!/usr/bin/env bash

mkdir -p toolchain
cd toolchain

saturn="$(dirname $(pwd))"

if [ -d "$saturn/toolchain/binutils/bin" ]; then
    exit 0;
fi

wget -nc https://ftp.gnu.org/gnu/binutils/binutils-2.31.tar.xz

tar xf binutils-2.31.tar.xz

cd binutils-2.31

patch < "$saturn/meta/binutils_patches/config.sub.patch"
cd bfd && patch < "$saturn/meta/binutils_patches/config.bfd.patch" && cd ..
cd ld && patch < "$saturn/meta/binutils_patches/configure.tgt.patch"
patch < "$saturn/meta/binutils_patches/Makefile.am.patch" && cd ..
cd gas && patch < "$saturn/meta/binutils_patches/gas_configure.tgt.patch" && cd ..

cp "$saturn/meta/binutils_patches/elf_i386_saturn.sh" ld/emulparams
cp "$saturn/meta/binutils_patches/elf_x86_64_saturn.sh" ld/emulparams

cd ld
automake
cd ../..

mkdir binutils_build
cd binutils_build

../binutils-2.31/configure --target=x86_64-saturn --prefix="$saturn/toolchain/binutils" --with-sysroot --disable-nls --disable-werror
make
make install