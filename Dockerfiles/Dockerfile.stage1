#: vim: set ft=dockerfile :

from bin2llvm-dev-stage0

add examples /usr/src/bin2llvm/examples
add harvesting-passes /usr/src/bin2llvm/harvesting-passes
add postprocess /usr/src/bin2llvm/postprocess
add pymodules /usr/src/bin2llvm/pymodules
add patches /usr/src/bin2llvm/patches

add scripts/build_stage1.sh /usr/src/bin2llvm/scripts/build_stage1.sh
add scripts/install.sh /usr/src/bin2llvm/scripts/install.sh

add bin2llvm.py /usr/src/bin2llvm/bin2llvm.py

# build stage 1
run cd /usr/src/bin2llvm \
	&& ./scripts/build_stage1.sh /usr/src/bin2llvm /usr/src/bin2llvm-build

add scripts /usr/src/bin2llvm/scripts

# install
run cd /usr/src/bin2llvm \
	&& mkdir -p /usr/local/bin2llvm/ \
	&& ./scripts/install.sh /usr/src/bin2llvm-build /usr/local/bin2llvm

run groupadd -r bin2llvm -g 999 \
	&& useradd -u 999 -r -g 999 -d /usr/local/bin2llvm -s /bin/bash bin2llvm



# testing dir
add tests /usr/local/bin2llvm/tests/
run chown -R 999:999 /usr/local/bin2llvm/
