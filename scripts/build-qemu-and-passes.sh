#!/bin/bash
# this script can be used to compile the passes required for LLVM

cd /mnt/store/projects/llvm-translator-new-build

# build quemu and quemu-debug
make -f ../llvm-translator-new/Makefile stamps/qemu-debug-make
make -f ../llvm-translator-new/Makefile

# build llvm for passes
make -f ../llvm-translator-new/Makefile stamps/llvm-cmake-configure
make -f ../llvm-translator-new/Makefile stamps/llvm-cmake-make
make -C llvm-release install
make -C llvm-cmake install

# build passes
make -f ../llvm-translator-new/Makefile stamps/translator_passes-release-configure
make -C translator_passes-release

cd -
