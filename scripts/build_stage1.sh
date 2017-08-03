#!/bin/bash
#
# Copyright 2017 The Bin2LLVM Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
################################################################################


# this will build all makefile
set -x

test ${#} -eq 2 || { echo $0 path-to-src-dir path-to-build-dir; exit -1 ; } ;
test "x${DO_NOT_BUILD_STAGE1}" != "x" && { echo do not build stage 1; exit -1 ; } ;

src_dir="${1}"
build_dir="${2}"

src_dir=$(realpath "${src_dir}")
build_dir=$(realpath "${build_dir}")

tp_dir=${src_dir}/third_party

export BUILD_ARCH=x86-64
export EXTRA_QEMU_FLAGS="--extra-cflags=-I${tp_dir}/cajun/include --extra-cxxflags=-I${tp_dir}/cajun/include"
export CAJUN_INCLUDE_DIR="${tp_dir}/cajun/include"

set -uex

mkdir -p "${build_dir}"
cd ${build_dir}

s2e_dir="${tp_dir}/s2e"

# install symlinks if needed
ln -fs "${src_dir}/harvesting-passes" "${s2e_dir}/qemu/s2e/Plugins/bin2llvm"

ln -fs "${src_dir}/harvesting-passes/JumpTableInfo.cpp" "${src_dir}/postprocess/translator/JumpTableInfo.cpp"
ln -fs "${src_dir}/harvesting-passes/JumpTableInfo.h" "${src_dir}/postprocess/translator/JumpTableInfo.h"


make -f ${src_dir}/third_party/s2e/Makefile

# configure llvm for cmake
make -f ${src_dir}/third_party/s2e/Makefile stamps/llvm-cmake-configure

# build llvm for cmake
make -f ${src_dir}/third_party/s2e/Makefile stamps/llvm-cmake-make

# install llvm
make -C llvm-release install

# install llvm-cmake
make -C llvm-cmake install

# this is needed to fix some llvm install issue
pfile="${src_dir}/patches/extra/001-llvm-native-fix-bits-cfg.diff"
patch -N -s -p0 --dry-run <  ${pfile} && patch -N -s -p0 < ${pfile}

# configure passes
passes_build_dir="${build_dir}/bin2llvm-postprocess-build"
CLANG_CXX="${build_dir}/llvm-native/Release/bin/clang++"
test -d "${passes_build_dir}" || mkdir -p "${passes_build_dir}"
cd "${passes_build_dir}"
cmake -G "Unix Makefiles" \
	-DLLVM_DIR="${build_dir}/llvm-cmake/share/llvm/cmake" \
	-DCMAKE_CXX_COMPILER="${CLANG_CXX}" \
	-DCAJUN_INCLUDE_DIR="${CAJUN_INCLUDE_DIR}" \
	${src_dir}/postprocess

# build passes
make

cd ${src_dir}
