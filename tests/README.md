# Testsuite for LLVM-translator

This folder contains the test-suite for the [bin2llvm] translator.(https://github.com/cojocar/bin2llvm).

## Building the tests

```bash
$ TOOLS_PREFIX=arm-none-eabi- make build
```

## Running tests

We use Cucumber (1.3.18) and Aruba (0.6.0) as test-runners, so you need to install them before
starting the checks (`gem install --user-install aruba --version 0.6.0` and
`gem install cucumber --version 1.3.18`).


Run the tests with:
```bash
$ BIN2LLVM_INSTALL_DIR=/usr/local/bin2llvm make check
```
