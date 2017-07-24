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

set -x

test ${#} -eq 2 || { echo $0 path-to-build-dir path-to-target-dir; exit -1 ; } ;

BUILD_DIR="${1}"
TARGET_DIR="${2}"

#BUILD_DIR="/mnt/projects/translator/bin2llvm-build/"
#TARGET_DIR="/tmp/bin2llvm-release/"


#src_dir="/mnt/projects/translator/bin2llvm/"
src_dir=$(pwd)

build_dir=$(realpath "${BUILD_DIR}")
target_dir=$(realpath "${TARGET_DIR}")
src_dir=$(realpath "${src_dir}")


#test -d "${target_dir}" &&  { echo "please remove ${target_dir}"; exit -1 ; } ;

bin_dir="${target_dir}/bin"
lib_dir="${target_dir}/lib"
mkdir -p "${target_dir}"
mkdir -p "${bin_dir}"
mkdir -p "${lib_dir}"

# qemu-system-arm

# -nodefconfig -L . must be passed to qemu
$(cd "${build_dir}"; cp -vp --parents qemu-release/*-s2e-softmmu/qemu-system-* "${bin_dir}")
$(cd "${build_dir}"; cp -vp --parents qemu-release/*-s2e-softmmu/op_helper.bc "${bin_dir}")

# llvm stuff
cp -vp "${build_dir}/opt/bin/opt" "${bin_dir}"
cp -vp "${build_dir}/opt/bin/llvm-as" "${bin_dir}"
cp -vp "${build_dir}/opt/bin/llvm-link" "${bin_dir}"
cp -vp "${build_dir}/opt/bin/llvm-dis" "${bin_dir}"
cp -vp "${build_dir}/opt/lib/"* "${lib_dir}"

# linky and translator
cp -vp "${build_dir}/bin2llvm-postprocess-build/linker/linky" "${bin_dir}"
cp -vp "${build_dir}/bin2llvm-postprocess-build/translator/translator.so" "${lib_dir}"
cp -vp "${src_dir}/postprocess/translator/mem-ops-alt.ll" "${lib_dir}"

# main script
cp -vp "${src_dir}/bin2llvm.py" "${bin_dir}"
cp -vpR "${src_dir}/pymodules/" "${bin_dir}/bin2llvm-pymodules"

cp -vp "${src_dir}"/examples/ls-example "${bin_dir}"
cp -vp "${src_dir}"/scripts/cleanup-output-dir.sh "${bin_dir}"
