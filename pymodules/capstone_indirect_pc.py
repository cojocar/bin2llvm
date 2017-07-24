#
# Copyright 2017 The bin2llvm Authors

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#     http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
###############################################################################

from capstone import *
from capstone.arm import *

import json
import sys

sys.path.append('/mnt/projects/translator/llvm-translator/script/')
from elf_parser import do_elf

def get_list_of_write_to_pc_insn(data, load_offset=0x0, verbose=False):
    md = Cs(CS_ARCH_ARM, CS_MODE_ARM | CS_MODE_MCLASS)
    md.detail = True

    align = 0

    ret = []
    for off_ in range(0, len(data), 4):
        off = off_ + align
        for insn in md.disasm(data[off:], offset = load_offset+off, count=1):
            isWriteToPC = False
            for op in insn.operands:
                try:
                    rn = insn.reg_name(op.reg)
                except:
                    continue

                if rn  == 'pc':
                    read_regs, write_regs = insn.regs_access()
                    for r in write_regs:
                        if r == op.reg:
                            isWriteToPC = True
            if isWriteToPC:
                if verbose:
                    print("0x%08x: %s %s" % (insn.address, insn.mnemonic.upper(), insn.op_str))
                ret.append(insn.address+off)
    return ret

def load_elf(path_to_elf='/mnt/projects/translator/llvm-translator-dist-run/linear-sync/spec-arm-clang-dynamic-bzip2'):
    info, thumb_file = do_elf(path_to_elf)
    #print(json.dumps(info, indent=1))
    ret = []
    for s in info['segments']:
        with open(s['file'], 'rb') as f:
            data = f.read()
        r = get_list_of_write_to_pc_insn(data, s['address'])
        ret.extend(r)
    return ret


def main():

    load_elf()
    with open('/tmp/translator-3LEyPw/seg-0.bin', 'rb') as f:
        data = f.read()

if __name__ == '__main__':
    main()
