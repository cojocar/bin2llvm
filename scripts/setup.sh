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

function checkout_and_clone()
{
	app=${1}
	url=${2}
	target_commit=${3}


	mkdir -p ${tp}
	target_dir=${tp}/${app}

	# clone (if needed)
	if [ -d ${target_dir}/.git ] ; then
		(cd ${target_dir} && git fetch)
	else
		git clone ${url} ${target_dir}
	fi

	# checkout the right version
	(cd ${target_dir} && git checkout ${target_commit})

	if [ -d patches/${app} ] ; then
		# apply patches
		for p in patches/${app}/*.patch; do
			pp=$(realpath "${p}")
			(cd ${target_dir} && patch -p1 < ${pp})
		done
	fi
}

tp=third_party
test -d ${tp} && { echo "please remove the ${tp} directory"; exit -1; }

src_dir=$(pwd)
#s2e_dir="${src_dir}/${tp}/s2e"

#### S2E
checkout_and_clone s2e https://github.com/dslab-epfl/s2e.git bb6760f
# install symlinks
#ln -fs "${src_dir}/harvesting-passes" "${s2e_dir}/qemu/s2e/Plugins/bin2llvm"
#
#ln -fs "${src_dir}/harvesting-passes/JumpTableInfo.cpp" "${src_dir}/postprocess/translator/JumpTableInfo.cpp"
#ln -fs "${src_dir}/harvesting-passes/JumpTableInfo.h" "${src_dir}/postprocess/translator/JumpTableInfo.h"

checkout_and_clone cajun https://github.com/cajun-jsonapi/cajun-jsonapi.git 24652f8
