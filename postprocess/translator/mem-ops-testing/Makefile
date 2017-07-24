# run this as a standalone from current directory
#
#
DIR=/mnt/projects/translator/llvm-translator
LLVMAS=$(DIR)-build/llvm-release/Release+Asserts/bin/llvm-as
MEMOPS=$(DIR)/translator_passes/translator/mem-ops-alt.ll
CLANG=$(DIR)-build/llvm-native/Release/bin/clang
F=mem-ops-testing

all: $(MEMOPS)
	$(LLVMAS) $(MEMOPS) -o /tmp/$(F)-llvm.bc
	$(CLANG) $(F).c /tmp/$(F)-llvm.bc -o $(F).elf
