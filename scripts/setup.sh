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

patch_apply_cmd="patch"
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
	(cd ${target_dir} && git checkout ${target_commit} -b ${app}-dev)

	if [ "${patch_apply_cmd}" = "git am" ]; then
		# make sure that git config user.email and user.name are set
		(cd ${target_dir}; \
			git config user.email || git config --local user.email "bin2llvm@github.com" ; \
			git config user.name || git config --local user.name "The bin2llvm Authors"; )
	fi

	if [ -d patches/${app} -a $(ls patches/${app}/*.patch 2>/dev/null | wc -l) -gt 0 ] ; then
		# apply patches
		for p in patches/${app}/*.patch; do
			pp=$(realpath "${p}")
			(cd ${target_dir} && \
				patch -N -s -p1 --dry-run <  ${pp} && \
				${patch_apply_cmd} -p1 < ${pp})
		done
	fi
}

tp=third_party
test -d ${tp} && { echo "please remove the ${tp} directory"; exit -1; }

src_dir=$(pwd)
#s2e_dir="${src_dir}/${tp}/s2e"

#### S2E
patch_apply_cmd="git am"
checkout_and_clone s2e https://github.com/dslab-epfl/s2e.git bb6760f
checkout_and_clone cajun https://github.com/cajun-jsonapi/cajun-jsonapi.git 24652f8
