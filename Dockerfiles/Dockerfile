from debian:jessie-slim

run set -x; \
	mkdir -p /usr/src/bin2llvm/ \
	&& apt-get update \
	&& echo 'ca-certificates build-essential git liblua5.1-0-dev libsdl1.2-dev libsigc++-2.0-dev binutils-dev libiberty-dev libc6-dev-i386' > /usr/src/bin2llvm/deps \
	&& apt-get install -y $(cat /usr/src/bin2llvm/deps) --no-install-recommends

run set -x; \
	apt-get install -y --no-install-recommends gcc-multilib g++-multilib nasm subversion flex bison
run set -x; \
	apt-get install -y --no-install-recommends wget cmake libgettextpo-dev

run set -x; \
	apt-get install -y --no-install-recommends python-llvm python-pip

run pip install pyelftools

add examples /usr/src/bin2llvm/examples
add harvesting-passes /usr/src/bin2llvm/harvesting-passes
add postprocess /usr/src/bin2llvm/postprocess
add pymodules /usr/src/bin2llvm/pymodules
add patches /usr/src/bin2llvm/patches

add scripts /usr/src/bin2llvm/scripts

add bin2llvm.py /usr/src/bin2llvm/bin2llvm.py

# do the setup
run cd /usr/src/bin2llvm \
	&& rm -rf third_party \
	&& ./scripts/setup.sh

# build & install
run cd /usr/src/bin2llvm \
	&& ./scripts/build.sh /usr/src/bin2llvm-build \
	&& mkdir -p /usr/local/bin2llvm/ \
	&& ./scripts/install.sh /usr/src/bin2llvm-build /usr/local/bin2llvm

run groupadd -r bin2llvm -g 999 \
	&& useradd -u 999 -r -g 999 -d /usr/local/bin2llvm -s /bin/bash bin2llvm


# testing dependencies
run set -x; \
	apt-get install -y --no-install-recommends rubygems ruby-dev gcc-arm-none-eabi

run set -x; \
	gem install aruba --version 0.6.0 && \
	gem install cucumber --version 1.3.18

# testing dir
add tests /usr/local/bin2llvm/tests/
run chown -R 999:999 /usr/local/bin2llvm/
