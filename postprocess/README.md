# LLVM Passes

These passes should be applied on the .bc file generated in the previous step
of binary translation. These passes improve the quality of the dissasembled
code.

## Building

Prepare the environment for the passes

1. ```cd ../llvm-translator-new-build```
2. ```make -f ../llvm-translator-new/Makefile```
3. ```make -f ../llvm-translator-new/Makefile stamps/llvm-cmake-configure```
4. ```make -f ../llvm-translator-new/Makefile stamps/llvm-cmake-make```
5. ```make -C llvm-release install```
6. ```make -C llvm-cmake install```

Build the passes

If you get this error: `error: no member named 'max_align_t' in the global
namespace`, please apply the patch from
[here](../llvm-translator-new-new/patches/fix-bits-cfg-passes.diff)
You should be able to use this command: `patch -p0   < ../llvm-translator-new-new/patches/fix-bits-cfg-passes.diff`.

1. ```make -f ../llvm-translator-new/Makefile stamps/translator_passes-release-configure```
2. ```make -C translator_passes-release/```

## Running
This is done automatically by the [main script translator](../script/run-translator.py).
```
/mnt/store/projects/llvm-translator-new-build/llvm-release/Release+Asserts/bin/opt \
	-load /mnt/store/projects/llvm-translator-new-build/translator_passes-release/translator/translator.so \
	s2e-last/translated_ll.bc -s2etransform -o /tmp/o.bc
```
