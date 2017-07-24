#!/bin/bash

set -ex

DOCKER="sudo docker"

tar cz -- Dockerfiles/Dockerfile $(cat Dockerfiles/file-list) | ${DOCKER} build -t bin2llvm-dev -f Dockerfiles/Dockerfile -

${DOCKER} build -t bin2llvm-release - < Dockerfiles/Dockerfile.release

# squash

container=$(${DOCKER} run -d bin2llvm-release true)
${DOCKER} export $container | ${DOCKER} import - bin2llvm-release-squashed


${DOCKER} save bin2llvm-release-squashed | gzip -9 > bin2llvm-release-squashed.tar.gz
