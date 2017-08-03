#: vim: set ft=dockerfile :

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

# testing dependencies
run set -x; \
	apt-get install -y --no-install-recommends rubygems ruby-dev gcc-arm-none-eabi

run set -x; \
	gem install aruba --version 0.6.0 && \
	gem install cucumber --version 1.3.18

add patches /usr/src/bin2llvm/patches

run mkdir -p /usr/src/bin2llvm/scripts

add scripts/setup.sh /usr/src/bin2llvm/scripts/setup.sh
add scripts/build_stage0.sh /usr/src/bin2llvm/scripts/build_stage0.sh

# do the setup
run cd /usr/src/bin2llvm \
	&& rm -rf third_party \
	&& ./scripts/setup.sh

# build stage 0
run cd /usr/src/bin2llvm \
	&& ./scripts/build_stage0.sh /usr/src/bin2llvm /usr/src/bin2llvm-build

