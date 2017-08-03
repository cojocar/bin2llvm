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

set -ux

regen_patches()
{
	app=${1}
	target_commit=${2}
	target_dir="${tp}/${app}"

	patch_dir=$(realpath "${src_dir}/patches/${app}")
	mkdir -p "${patch_dir}"

	(cd "${target_dir}" && git format-patch -o "${patch_dir}" ${target_commit}..HEAD)
}

tp=third_party

src_dir=$(pwd)

regen_patches s2e bb6760f
regen_patches cajun 24652f8
