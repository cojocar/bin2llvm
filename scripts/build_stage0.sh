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


# this will build all the llvm&klee dependencies
set -uex

test ${#} -eq 2 || { echo $0 path-to-src-dir path-to-build-dir; exit -1 ; } ;

src_dir="${1}"
build_dir="${2}"

src_dir=$(realpath "${src_dir}")
build_dir=$(realpath "${build_dir}")

tp_dir=${src_dir}/third_party

export BUILD_ARCH=x86-64
export EXTRA_QEMU_FLAGS="--extra-cflags=-I${tp_dir}/cajun/include --extra-cxxflags=-I${tp_dir}/cajun/include"
export CAJUN_INCLUDE_DIR="${tp_dir}/cajun/include"

mkdir -p "${build_dir}"
cd ${build_dir}

make -f ${src_dir}/third_party/s2e/Makefile stamps/tools-release-make
make -f ${src_dir}/third_party/s2e/Makefile stamps/klee-release-make

cd ${src_dir}
