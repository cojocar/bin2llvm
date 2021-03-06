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

test ${#} -eq 1 || { echo $0 path-to-build-dir; exit -1 ; } ;

BUILD_DIR="${1}"

src_dir=$(pwd)

src_dir=$(realpath "${src_dir}")
build_dir=$(realpath "${BUILD_DIR}")


set -uex

./scripts/build_stage0.sh "${src_dir}" "${build_dir}"

./scripts/build_stage1.sh "${src_dir}" "${build_dir}"

