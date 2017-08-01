#!/bin/bash

set -ex

DOCKER="sudo docker"

# build stage 0
tar cz -- Dockerfiles/Dockerfile.stage0 $(cat Dockerfiles/file-list) | ${DOCKER} build -t bin2llvm-dev-stage0 -f Dockerfiles/Dockerfile.stage0 -

# squash stage 0 (bin2llvm-dev-stage0-squashed should be pushed to docker.io/cojocar/bin2llvm-dev-stage0)
# useful for CI
container=$(${DOCKER} run -d bin2llvm-dev-stage0 true)
${DOCKER} export $container | ${DOCKER} import - bin2llvm-dev-stage0-squashed
#### docker tag bin2llvm-dev-stage0-squashed cojocar/bin2llvm-dev-stage0
#### docker push cojocar/bin2llvm-dev-stage0

# build stage 1
tar cz -- Dockerfiles/Dockerfile.stage1 $(cat Dockerfiles/file-list) | ${DOCKER} build -t bin2llvm-dev -f Dockerfiles/Dockerfile.stage1 -

${DOCKER} build -t bin2llvm-release - < Dockerfiles/Dockerfile.release

# squash the release (bin2llvm-release-squashed should be pushed to docker.io/cojocar/bin2llvm)
container=$(${DOCKER} run -d bin2llvm-release true)
${DOCKER} export $container | ${DOCKER} import - bin2llvm-release-squashed
#### docker tag bin2llvm-release-squashed cojocar/bin2llvm
#### docker push cojocar/bin2llvm


${DOCKER} save bin2llvm-release-squashed | gzip -9 > bin2llvm-release-squashed.tar.gz
