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

from elftools.elf.elffile import ELFFile
from elftools.elf.elffile import ELFFile
from elftools.elf.enums import ENUM_P_TYPE
import subprocess
import os
import json

import logging

logging.basicConfig()
log = logging.getLogger("translator")

def save_chunk(dst_path, src_path, offset, size):
    log.debug("[Translator] Saving chunk to %s [off=%08x,size=%08x]" % (dst_path, offset, size))
    dst = open(dst_path, 'wb')
    with open(src_path, 'rb') as f:
        f.seek(offset)
        data = f.read(size)
        # add padding
        if len(data) < size:
            data = data + '\x00' * (size - len(data))
        dst.write(data[:size])
    dst.close()


def do_elf(path_to_elf, out_dir='/tmp'):
    """This function reads information from elf file and translates it to
    appropiate config structure.
    """

    ret = {}
    elf = ELFFile(open(path_to_elf, 'rb'))
    if 'ARM' != elf.get_machine_arch():
        raise Exception("Architecture not supported")

    ret['architecture'] = 'arm'
    ret['cpu_model'] = 'arm926'
    ret['endiannes'] = 'little' if elf.little_endian else 'big'
    ret['entry_address'] = [elf.header.e_entry]

    thumb_targets = []
    targets = []
    for sec in elf.iter_sections():
        if sec.name.startswith(b'.symtab'):
            log.info("[Translator] binary contains symbols! Using those instead of the single entry")
            ret['entry_address'] = []
            # nm can run on any type of elf binary
            p = subprocess.Popen(['nm', path_to_elf], stdout=subprocess.PIPE)
            out, _ = p.communicate()
            for l in out.split('\n'):
                try:
                    addr, t, name = l.split(' ')
                except:
                    continue
                if (t == 't' or t == 'T') and not name.startswith('$'):
                    targets.append(int(addr, 16))

            # call readelf -s for getting the thumb bit
            # somehow, the $a and $t are not always generated?
            p = subprocess.Popen(['readelf', '-s', path_to_elf], \
                    stdout=subprocess.PIPE)
            out, _ = p.communicate()
            for l in out.split('\n'):
                try:
                    _, addr, _, t, _, _, _, name = l.split()
                except:
                    #print("QQ: %s: %d" % (l, len(l.split(' '))))
                    #print(str(l.split(None)))
                    continue
                if t != 'FUNC':
                    continue
                jumpPC = int(addr, 16)
                if jumpPC & 1 == 0x1:
                    thumb_targets.append(jumpPC & -2)

    segments = []
    cnt = 0
    mapped_targets = []
    mapped_thumb_targets = []

    for i in range(elf.num_segments()):
        seg = elf.get_segment(i)
        if seg.header.p_type != 'PT_LOAD':
            continue

        #print(dir(seg.header))
        assert(seg.header.p_paddr == seg.header.p_vaddr)
        padding = seg.header.p_paddr % 4096
        #assert(seg.header.p_paddr % 4096 == 0)

        new_section = {}
        s = max(seg.header.p_memsz, seg.header.p_filesz)

        # round up to 4k
        if s % 4096 != 0:
            s = 4096*int((s+4096)/4096)

        s += padding

        # round up to 4k
        if s % 4096 != 0:
            s = 4096*int((s+4096)/4096)

        # build segment info
        segm_name = 'seg-'+str(cnt)+'.bin'
        segm_file = os.path.join(out_dir, segm_name)
        offset = seg.header.p_offset-padding
        assert(offset >= 0)

        segm_desc = {}
        segm_desc['file'] = segm_file
        segm_desc['size'] = s
        segm_desc['address'] = seg.header.p_paddr - padding
        segm_desc['name'] = segm_name

        # save chunk
        save_chunk(segm_file, path_to_elf, offset, s)
        cnt += 1
        segments.append(segm_desc)

        log.debug("[Translator] loaded %s%08x@%08x" % \
                (seg.header.p_type, seg.header.p_paddr, \
            seg.header.p_offset))

        def inside_segment(e):
            return e >= segm_desc['address'] and \
                e < (segm_desc['address'] + \
                    segm_desc['size'])
        # filter data
        map(mapped_targets.append, filter(inside_segment, \
                targets))
        map(mapped_thumb_targets.append, filter(inside_segment, \
                thumb_targets))

    ret['segments'] = segments

    # unique
    mapped_thumb_targets = sorted(list(set(mapped_thumb_targets)))
    mapped_targets = sorted(list(set(mapped_targets)))
    log.debug("[Translator] elf: %d entries and %d thumb bits" % \
            (len(mapped_targets), len(mapped_thumb_targets)))

    if len(mapped_thumb_targets) > 0:
        fout = os.path.join(out_dir, "is-thumb-initial.json")
        with open(fout, 'wt') as f:
            f.write(json.dumps(mapped_thumb_targets))
    else:
        fout = None

    map(ret['entry_address'].append, mapped_targets)
    return ret, fout

