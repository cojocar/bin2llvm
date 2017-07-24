#!/bin/bash

/mnt/store/projects/llvm-translator-new-build/llvm-release/Release+Asserts/bin/opt  -load /mnt/store/projects/llvm-translator-new-build/translator_passes-release/translator/translator.so \
	-transfrombbtovoid \
	-rmexterstoretopc \
	-s2edeleteinstructioncount \
	-fixoverlappedbbs \
	-armmarkcall \
	-dce -gvn -dce \
	-armmarkreturn \
	-armmarkjumps \
	-markfuncentry \
	-rmbranchtrace \
	-replaceconstantloads -memory /tmp/translator-9iqF3W/seg-0.bin@0x0 \
	-buildfunctions -save-funcs /tmp/funcs.bc \
	${1} -o /tmp/o.bc
