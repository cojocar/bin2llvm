# The bin2llvm Project [![Build Status](https://travis-ci.org/cojocar/bin2llvm.svg?branch=master)](https://travis-ci.org/cojocar/bin2llvm)

This is an [S2E](https://github.com/dslab-epfl/s2e/) based binary-to-LLVM
translator. It converts any binary code to LLVM code. The resulting LLVM module
contains functions. Some, control flow details are recovered.

## Overview

The idea is to reuse components from S2E to achieve the translation to LLVM.
Rougly, qemu translates from binary to TCG and S2E translates from TCG to LLVM.
Plugins were added to perform the recursive disassembly of the binary. The
*raw* LLVM code is then fed to a set of external LLVM passes. The purpose of
these step is to add more details about the extracted code, concretely, basic
blocks are grouped in functions.
It is mainly tested on the ARM architecture.
*bin2llvm* is a best effort tool, it will try to translate as much as possible
and then link the LLVM code in a final file.

## Running the Docker image

```bash
$ docker pull docker.io/cojocar/bin2llvm
$ # run one example binary
$ docker run --rm -t docker.io/cojocar/bin2llvm /bin/bash -c "/usr/local/bin2llvm/bin/bin2llvm.py --file /usr/local/bin2llvm/bin/ls-example"
$ # run the tests
$ docker run --rm -t docker.io/cojocar/bin2llvm /bin/bash -c "cd /usr/local/bin2llvm/tests; BIN2LLVM_INSTALL_DIR=/usr/local/bin2llvm make;"
```

## How to build, install & run from the source tree

### Dependencies

Consult the [Dockerfile](./Dockerfiles/Dockerfiles) for the list of dependencies.

### Building (outside Docker)
```bash
$ ./scripts/setup.sh # this will copy some dependencies in the third_party directory
$ ./scripts/build.sh ../bin2llvm-build
$ ./scripts/install.sh ../bin2llvm-build ../bin2llvm-install
```

### (optionally) Building the Docker image
```bash
$ ./scripts/build_docker.sh
```
This will result in `bin2llvm-dev` and in `bin2llvm-release-squashed` images.

### Running

```bash
$ cd ../bin2llvm-install && ./bin/bin2llvm.py --file ./bin/ls-example
Press Ctrl+C
INFO:bin2llvm:Using /tmp/bin2llvm-W4yJvU as temp_dir
INFO:bin2llvm:Use entry: 0x00009a74
INFO:bin2llvm:Use entry: 0x00009fa8
INFO:bin2llvm:Use entry: 0x0000c470
INFO:bin2llvm:Use entry: 0x0000c4d0
INFO:bin2llvm:Use entry: 0x0000c514
INFO:bin2llvm:Use entry: 0x0000c560
....
INFO:bin2llvm:Use entry: 0x00000000
WARNING:bin2llvm:(passes) crashed with entry: 0x00000000
INFO:bin2llvm:FINAL output is in /tmp/bin2llvm-W4yJvU/final.bc (370 functions)
```
The final bit code is `${OUT_DIR}/final.bc`

## Testing

```bash
$ cd ./tests && BIN2LLVM_INSTALL_DIR=$(realpath ../../bin2llvm-install) make
```

See the [test](./test) directory for more details.

---

# bin2llvm in practice

The following works are using *bin2llvm*:
- Cojocar, Lucian, Taddeus Kroes, and Herbert Bos. "[JTR: A Binary Solution for Switch-Case Recovery](https://link.springer.com/chapter/10.1007/978-3-319-62105-0_12)" International Symposium on Engineering Secure Software and Systems. Springer, Cham, 2017.
- Cojocar, Lucian, et al. "[Pie: parser identification in embedded systems.](www.cs.vu.nl/~herbertb/papers/pie_acsac15.pdf)" Proceedings of the 31st Annual Computer Security Applications Conference. ACM, 2015.
