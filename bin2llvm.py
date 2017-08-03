#!/usr/bin/env python

# Copyright 2017 The bin2llvm Authors

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
###############################################################################

from elftools.elf.elffile import ELFFile
from elftools.elf.enums import ENUM_P_TYPE

import argparse
import tempfile
import os.path
import os
import json
import subprocess
import sys
import traceback

import logging

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'pymodules'))
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'bin2llvm-pymodules'))

from elf_parser import do_elf
from elf_parser import save_chunk
from coverage import CoverageStatusQemu
from paths import TranslatorPaths

logging.basicConfig()
log = logging.getLogger("bin2llvm")

def get_new_discovered_from_json(path):
    try:
        with open(path, 'rb') as f:
            return json.loads(f.read())
    except:
        return []

def merge_arm_thumb_bit(dst_file, srca_file, srcb_file):
    try:
        with open(srca_file, 'rb') as f:
            a = set(json.loads(f.read()))
    except (IOError, ValueError):
        a = set([])
    try:
        with open(srcb_file, 'rb') as f:
            b = set(json.loads(f.read()))
    except (IOError, ValueError):
        b = set([])
    with open(dst_file, 'wb') as f:
        f.write(json.dumps(list(a.union(b))))

def run_translator(tmp_dir, machine_path,
        translator_cfg_path, cnt=0):
    if log.getEffectiveLevel() == logging.DEBUG:
        console = open(os.path.join(tmp_dir, 'run_translator.log'), 'at')
    else:
        console = open(os.devnull, 'w')
    ret = True
    curr_path = os.getcwd()
    cmd = ''
    cmd += translator_path + ' '
    # -L path, path should contain the op_helper.bc file
    cmd += "-nodefconfig -L %s " % os.path.dirname(translator_path)
    cmd += "-M configurable -kernel %s " % machine_path
    cmd += "-nographic -monitor /dev/null -net none "
    cmd += "-s2e-config-file %s " % translator_cfg_path
    cmd += "-s2e-verbose -generate-llvm -D %s -d in_asm" % \
            (os.path.join(tmp_dir, 'qemu-%d.log' % cnt))
    log.debug('run_translator: "%s"' % cmd)
    os.chdir(tmp_dir)
    try:
        subprocess.check_call(cmd.split(' '), \
                stdout=console, stderr=console)
    except subprocess.CalledProcessError:
        log.debug('run_translator failed: ' + cmd)
    except OSError:
        log.debug('run_translator failed exe: ' + cmd)
        ret = False
    finally:
        try:
            console.close()
        except:
            pass
    os.chdir(curr_path)
    return ret

def get_replace_pass_cfg(cfg):
    ret = ''
    for seg in cfg['segments']:
        ret += '-memory %s@0x%08x ' % \
                (seg['file'], seg['address'])
    return ret

def init_path(endianness='little'):
    # this file is ${prefix}/bin/bin2llvm.py
    P = TranslatorPaths(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

    global so_path
    global opt_path
    global as_path
    global link_path
    global translator_path
    global linker_path
    global path_ll_for_helpers
    global linker_lib

    so_path = P.get_translator_so()
    opt_path = P.get_llvm_opt()
    as_path = P.get_llvm_as()
    link_path = P.get_llvm_link()
    translator_path = P.get_qemu(endianness)
    linker_path = P.get_linker()
    path_ll_for_helpers = P.get_ll_helpers()
    linker_lib = P.get_lib()

def run_passes_pre(raw_llvm, out_funcs, out_remaining, cfg, jump_table_file=None):
    if log.getEffectiveLevel() == logging.DEBUG:
        console = open(os.path.join(os.path.dirname(out_funcs), 'run_passes_pre.log'), 'at')
    else:
        console = open(os.devnull, 'w')
    cmd = ''
    cmd += opt_path + ' '
    cmd += '-load %s ' % so_path

    cmd +=  "-mem2reg "
    cmd +=  "-transfrombbtovoid "
    #cmd +=  "-dce "
    #cmd +=  "-s2edeleteinstructioncount "
    #cmd +=  "-dce "
    #cmd +=  "-fixoverlappedbbs "
    cmd += "-tag-inst-pc "
    cmd += "-rmexterstoretopc "
    cmd += "-s2edeleteinstructioncount "
    cmd +=  "-fixoverlappedbbs "
    #cmd += "-fix-overlaps "
    cmd += "-dce "
    cmd += "-internalize-globals "
    replace_cst_cfg = get_replace_pass_cfg(cfg)
    if jump_table_file is None:
        cmd +=  "-replaceconstantloads %s" % (replace_cst_cfg)
    else:
        cmd +=  "-replaceconstantloads -jump-table-info %s %s" % (jump_table_file, replace_cst_cfg)
    cmd +=  "-dce "
    cmd +=  "-armmarkcall "
    cmd +=  "-dce -gvn -dce "
    cmd +=  "-armmarkreturn "
    cmd +=  "-armmarkjumps "
    cmd +=  "-markfuncentry "
    cmd +=  "-rmbranchtrace "
    if cfg['endianness'] != 'little':
        cmd += "-is-big-endian "
    cmd +=  "-buildfunctions -save-funcs %s " % out_funcs

    cmd += "%s -o %s" % (raw_llvm, out_remaining)

    try:
        subprocess.check_call(cmd.split(' '), \
                stdout = console, stderr = console)
    except subprocess.CalledProcessError:
        log.debug("run_passes_pre failed: " + cmd)
        return False
    finally:
        try:
            console.close()
        except:
            pass
    return True

def run_passes_post(in_bc, out_bc):
    if log.getEffectiveLevel() == logging.DEBUG:
        console = open(os.path.join(os.path.dirname(in_bc), 'run_passes_post.log'), 'at')
    else:
        console = open(os.devnull, 'w')
    cmd = ''

    cmd += opt_path + ' '
    cmd += '-load %s ' % so_path
    cmd += "-dce "
    cmd += "-internalize-globals "
    cmd += "-basicaa "
    cmd += "-sink "
    cmd += "-constprop "
    cmd += "-gvn "
    cmd += "-dce "
    cmd += "-funcrename "
    cmd += "-alias-global-bb "
    cmd += "-dce "
    cmd += "%s -o %s" % (in_bc, out_bc)

    try:
        subprocess.check_call(cmd.split(' '), \
                stdout = console, stderr = console)
    except subprocess.CalledProcessError:
        log.debug("run_passes_post failed: " + cmd)
        return False
    finally:
        try:
            console.close()
        except:
            pass
    return True

def run_indirect_solver(in_bc, out_bc, out_new_targets_json):
    # run opt -gvn -basicaa
    if log.getEffectiveLevel() == logging.DEBUG:
        console = open(os.path.join(os.path.dirname(in_bc), 'run_indirect_solver.log'), 'at')
    else:
        console = open(os.devnull, 'w')
    opt_bc = out_bc+'.opt.for.insidrect.bc'

    cmd = ''
    cmd += opt_path + ' '
    cmd += "-load %s " % so_path

    cmd += "-gvn "
    cmd += "-basicaa "
    cmd += "%s -o %s" % (in_bc, opt_bc)

    try:
        subprocess.check_call(cmd.split(' '), \
                stdout = console, stderr = console)
    except subprocess.CalledProcessError:
        log.debug("run_indirect_solver (opt) failed: " + cmd)
        return False
    finally:
        try:
            console.close()
        except:
            pass

    # run our custom pass
    cmd = ''
    cmd += opt_path + ' '
    cmd += "-load %s " % so_path
    cmd += "-solveindirectsingle -optfile %s -solvedfile %s " % \
            (opt_bc, out_new_targets_json)
    cmd += "-dce "
    cmd += "%s -o %s" % (in_bc, out_bc)

    if log.getEffectiveLevel() == logging.DEBUG:
        console = open(os.path.join(os.path.dirname(in_bc), 'run_indirect_solver.log'), 'at')
    else:
        console = open(os.devnull, 'w')
    try:
        subprocess.check_call(cmd.split(' '), \
                stdout = console, stderr = console)
    except subprocess.CalledProcessError:
        log.debug("run_indirect_solver (passes) " + cmd)
        return False
    finally:
        try:
            console.close()
        except:
            pass
    return True

def run_arm_dump_thumb_bit(in_bc, out_json):
    if log.getEffectiveLevel() == logging.DEBUG:
        console = open(os.path.join(os.path.dirname(in_bc), 'run_arm_dump_thumb_bit.log'), 'at')
    else:
        console = open(os.devnull, 'w')
    #so_path = os.path.join(translator_build_dir, \
    #        "translator_passes-release/translator/translator.so")
    #so_path = P.get_translator_so()
    #opt_path = os.path.join(translator_build_dir, \
    #        "llvm-release/Release+Asserts/bin/opt")
    #opt_path = P.get_llvm_opt()

    cmd = ''
    cmd += opt_path + ' '
    cmd += "-load %s " % so_path
    cmd += "-armdumpthumbbit -outjson %s " % (out_json)
    cmd += "%s -o /dev/null" % (in_bc)

    try:
        subprocess.check_call(cmd.split(' '), \
                stdout=console, stderr=console)
    except subprocess.CalledProcessError:
        log.debug("run_arm_dump_thumb_bit failed: " + cmd)
        return False
    finally:
        try:
            console.close()
        except:
            pass
    return True

def generate_helper_bc(temp_dir):
    global helper_bc_path

    if log.getEffectiveLevel() == logging.DEBUG:
        console = open(os.path.join(temp_dir, 'compile_helpers.log'), 'at')
    else:
        console = open(os.devnull, 'w')


    cmd = ''
    cmd += as_path + ' '
    cmd += path_ll_for_helpers + ' '
    helper_bc_path = os.path.join(temp_dir, os.path.basename(path_ll_for_helpers)+'.bc')
    cmd += '-o ' + helper_bc_path
    try:
        subprocess.check_call(cmd.split(' '), \
                stdout=console, stderr=console)
    except subprocess.CalledProcessError:
        log.debug("run_indirect_solver (asm) failed: " + cmd)
        return False
    finally:
        try:
            console.close()
        except:
            pass


def run_inline_mmu_helpers(temp_dir, in_bc, out_bc):

    cmd = ''
    cmd += link_path + ' '
    cmd += helper_bc_path + ' '
    linked_bc_no_opt = os.path.join(temp_dir, os.path.basename(out_bc)+'no-opt.bc')
    cmd += in_bc + ' '
    cmd += '-o ' + linked_bc_no_opt
    if log.getEffectiveLevel() == logging.DEBUG:
        console = open(os.path.join(os.path.dirname(in_bc), 'run_inline_mmu_helpers.log'), 'at')
    else:
        console = open(os.devnull, 'w')
    try:
        subprocess.check_call(cmd.split(' '), \
                stdout=console, stderr=console)
    except subprocess.CalledProcessError:
        log.debug("run_inline_mmu_helpers (opt) failed: " + cmd)
        return False
    finally:
        try:
            console.close()
        except:
            pass

    cmd = ''
    cmd += opt_path + ' '
    cmd += '-always-inline '
    cmd += linked_bc_no_opt + ' '
    cmd += '-o ' + out_bc
    if log.getEffectiveLevel() == logging.DEBUG:
        console = open(os.path.join(os.path.dirname(in_bc), 'run_inline_mmu_helpers.log'), 'at')
    else:
        console = open(os.devnull, 'w')
    try:
        subprocess.check_call(cmd.split(' '), \
                stdout=console, stderr=console)
    except subprocess.CalledProcessError:
        log.debug("run_inline_mmu_helpers (passes) failed: " + cmd)
        return False
    finally:
        try:
            console.close()
        except:
            pass

    return True

def run_custom_linker(linker_path, func_bcs, out_file):
    if log.getEffectiveLevel() == logging.DEBUG:
        console = open(os.path.join(os.path.dirname(out_file), 'run_custom_linker.log'), 'at')
    else:
        console = open(os.devnull, 'w')
    cmd = ''
    cmd += linker_path + ' '
    cmd += ' '.join(func_bcs)
    cmd += ' ' + out_file

    try:
        lib_env = os.environ.copy()
        lib_env['LD_LIBRARY_PATH'] = linker_lib
        subprocess.check_call(cmd.split(' '), env = lib_env, \
                stdout=console, stderr=console)
    except subprocess.CalledProcessError:
        log.debug('run_custom_linker failed: ' + cmd)
        return False
    finally:
        try:
            console.close()
        except:
            pass
    return True

def write_machine_cfg(dst_path, arch, cpu, end, entry, segments):
    """
{
    "entry_address": 0, 
    "memory_map": [{
        "map": [{
            "permissions": "rwx",
            "type": "code",
            "address": 0
        }],
        "name": "flash", 
        "file": "./multiple-func.armle.c.bin",
        "size": 4096
    }
    ],

    "architecture": "arm",
    "cpu_model": "arm926",
    "devices": [],
    "endianess": "little"
}
"""
    #"cpu_model": "cortex-a8",
    memory_map = []
    for s in segments:
        map_entry = {}
        map_entry['name'] = s['name']
        map_entry['file'] = s['file']
        map_entry['size'] = s['size']
        map_entry['map'] = [{'permissions':'rwx', 'type' : 'code', 'address': s['address']}]
        memory_map.append(map_entry)

    machine_cfg = {}
    machine_cfg['entry_address'] = entry
    machine_cfg['architecture'] = arch
    machine_cfg['cpu_model'] = cpu
    machine_cfg['endianess'] = end
    machine_cfg['memory_map'] = memory_map
    #print(json.dumps(machine_cfg, indent=4))
    log.debug("[Translator] Writing machine file to %s" % dst_path)
    with open(dst_path, 'wt') as f:
        f.write(json.dumps(machine_cfg, indent=4))

def pairIfNotNone(desc, val):
    if val == None:
        return ''
    return '\t\t\t%s = "%s",\n'  % \
            (desc, val)

def write_tranlator_cfg(dst_path, segments, \
        already_file=None, \
        isThumbIn=None, \
        isThumbOut=None, \
        jumpTableInfoPath=None):
    constantMemoryRanges = "\tconstantMemoryRanges = {\n"
    for s in segments:
        constantMemoryRanges += """\t\t%s = {
\t\t\taddress = %s,
\t\t\tsize    = %s
\t\t}%s\n""" % (s['name'].replace('-', '_').replace('.', '_'),
            hex(s['address']),
            hex(s['size']),
            ',' if s != segments[-1] else '')
    constantMemoryRanges += "\t}\n"

    #print(constantMemoryRanges)
    cfg = """
s2e = {
    kleeArgs = {
        -- Pick a random path to execute among all the
        -- available paths.
        "--use-random-path=true",

        "--print-llvm-instructions",
        "--keep-llvm-functions", 
    }
}

plugins = {
    -- Enable a plugin that handles S2E custom opcode
    "RecursiveDescentDisassembler"
}

pluginsConfig = {
    RecursiveDescentDisassembler = {
        verbose = false,
        printFunctionBeforeOptimization = false,
        printFunctionAfterOptimization = false,
        %s
        %s
        %s
        %s
        %s
    }
}
""" % (pairIfNotNone('initialAlreadyVisited', already_file),\
        pairIfNotNone('isThumbIn', isThumbIn), \
        pairIfNotNone('isThumbOut', isThumbOut), \
        pairIfNotNone('jumpTableInfoPath', jumpTableInfoPath), \
        constantMemoryRanges)
    log.debug("[Translator] Saving translator cfg to: %s" % dst_path)
    with open(dst_path, 'wt') as f:
        f.write(cfg)


def do_raw_arm_one(path_to_raw, load_address):
    """This function generates the config for an raw binary for ARM
    architecture.
    """

    dst_path = os.path.join(args.temp_dir, os.path.basename(path_to_raw).replace('@', '_'))
    real_size = os.path.getsize(path_to_raw)
    padding = 4096 - (real_size % 4096)
    if real_size % 4096:
        padded_size = real_size + padding
    else:
        padded_size = real_size
    save_chunk(dst_path, path_to_raw, 0, padded_size)

    segm_desc = {}
    segm_desc['file'] = dst_path
    segm_desc['size'] = padded_size
    segm_desc['address'] = load_address
    segm_desc['name'] = 'file_%08x' % load_address

    return segm_desc


def do_raw_arm_many(paths_to_raw, load_addresses):
    """This function generates the config for a set of raw binary for ARM
    architecture.
    """
    assert(type(paths_to_raw) == list)
    assert(type(load_addresses) == list)
    assert(len(paths_to_raw) == len(load_addresses))

    ret = {}
    ret['architecture'] = 'arm'
    ret['cpu_model'] = 'arm926'

    segments = []

    for idx in range(len(paths_to_raw)):
        segm_desc = do_raw_arm_one(paths_to_raw[idx], load_addresses[idx])
        segments.append(segm_desc)

    ret['segments'] = segments

    return ret


def write_configs(machine_file, translator_file, cfg):
    write_machine_cfg(machine_file, \
            cfg['architecture'], cfg['cpu_model'], \
            cfg['endianness'], cfg['entry_address'], cfg['segments'])
    write_tranlator_cfg(translator_file, cfg['segments'])

should_continue = True
import signal
import sys

def signal_handler(signal, frame):
    print('You pressed Ctrl+C!')
    global should_continue
    should_continue = False

def main():
    parser = argparse.ArgumentParser("Run the translator")
    parser.add_argument("--file", metavar="FILE", \
            action='append', \
            help="Use this file as input for the translator",
            required=True)
    wd_group = parser.add_mutually_exclusive_group(required=False)
    wd_group.add_argument("--temp-dir", metavar="TMP", \
            help="Use this directory as a temporary directory", \
            required=False)
    wd_group.add_argument("--temp-dir-base", metavar="TMP-BASE", \
            help="Use this directory as a temporary directory base", \
            default=None, \
            required=False)
    parser.add_argument("--type", type=str, required=False, \
            choices=['elf', 'pe', 'raw-arm-le', 'raw-arm-be', 'raw-x86', 'raw-x86-64'], \
            default='elf', \
            help="Type of input. Default: elf.")
    parser.add_argument("--entry", action='append', required=False, \
            help="Specify one or many entry points. If entry starts with @ then \
            the entries are read from a json file. If none specified we will \
            use the \ one from elf.")
    parser.add_argument("--load-address", required=False, action='append', \
            help="If raw is specified, then the load address is required. Default = entry.")
    parser.add_argument("--out", required=False, \
            help="Final bc file output path. Default=${temp.dir}/final.bc")
    parser.add_argument("--verbose", "-v", action='store_true', \
            default=False,
            help="Enable verbose output.")
    parser.add_argument("--jump-table-file", "-j", required=False, \
            help="Json file with detected jump tables.")
    parser.add_argument("--remove-lock", "-k", action='store_true', \
            default=False,
            help="Remove ${tmp_dir}/lock file when done.")
    parser.add_argument("--thumb-bits-file", "-t", required=False, \
            help="Json file with a list of entries that we know that are in thumb mode")

    global args
    args = parser.parse_args()
    #print args

    if args.temp_dir is None:
        args.temp_dir = tempfile.mkdtemp(prefix='bin2llvm-', dir=args.temp_dir_base)

    if args.verbose is True:
        log.setLevel(logging.DEBUG)
    else:
        log.setLevel(logging.INFO)

    log.info("Using %s as temp_dir" % args.temp_dir)

    # use absolute path
    args.temp_dir = os.path.abspath(args.temp_dir)
    if args.jump_table_file is not None:
        args.jump_table_file = os.path.abspath(args.jump_table_file)

    # save args
    with open(os.path.join(args.temp_dir, 'args'), 'wt') as f:
        f.write(str(args))

    # parse files
    if args.type == 'elf':
        assert(args.load_address is None)
        assert(type(args.file) == list)
        assert(len(args.file) == 1)
        cfg, isThumbIn = do_elf(args.file[0], args.temp_dir)
    elif args.type.startswith('raw-arm-'):
        # we should have at least one entry for raw files
        assert(args.entry is not None)
        assert(len(args.entry) == 1 or args.load_address is not None)
        if args.load_address is None:
            args.load_address = [args.entry[0]]

        assert(type(args.file) == list)
        assert(len(args.load_address) == len(args.file))
        end = 'little' if args.type.endswith('le') else 'big'
        cfg = do_raw_arm_many(args.file, \
                map(lambda str_addr: int(str_addr.replace('_', ''), 16), args.load_address))
        cfg['endianness'] = end
    else:
        raise Exception("Type not supported yet.")

    # Parse entries
    if args.entry is not None:
        in_entry_list = []
        for e in args.entry:
            if e.startswith('@'):
                # read from json file:
                with open(e[1:], 'rt') as f:
                    d = json.loads(f.read())
                    assert(d.__class__ == list)
                    map(in_entry_list.append, d)
            else:
                in_entry_list.append(int(e, 16))
        cfg['entry_address'] = in_entry_list

    log.debug("[Translator] Using entry(s): %s" % (map(hex, cfg['entry_address'])))

    # write cfgs
    machine_file = os.path.join(args.temp_dir, 'machine.json')
    translator_file = os.path.join(args.temp_dir, 'translator.json')

    cov = CoverageStatusQemu()
    cnt = 0
    func_bcs = []
    try:
        isThumbIn
    except UnboundLocalError:
        isThumbIn = None

    if args.thumb_bits_file is not None:
        if isThumbIn is not None:
            log.warning("[Translator] override thumb bits by the readelf")
        isThumbIn = os.path.abspath(args.thumb_bits_file)

    #entryQueue = [0x00008b60] + cfg['entry_address']
    entryQueue = cfg['entry_address']
    #entryQueue = [0x00008c98]
    head = 0
    global should_continue
    should_continue = True
    init_path(cfg['endianness'])
    while head < len(entryQueue) and should_continue:
        #log.info("addresses visited: " + str(len(cov.getAlreadyExplored())))
        e = entryQueue[head]
        head += 1
        if cov.visited(e):
            log.debug("[Translator] already visited 0x%08x" % e)
            continue
        #write_configs(machine_file, translator_file, cfg, alreadyExplored)
        log.info("Use entry: 0x%08x" % (e))
        alreadyExplored = cov.get_already_explored_intervals()
        write_machine_cfg(machine_file, \
                cfg['architecture'], cfg['cpu_model'], \
                cfg['endianness'], e, cfg['segments'])
        already_file = os.path.join(args.temp_dir,\
                'already-explored-%d.json' % cnt)
        with open(already_file, 'wt') as f:
            f.write(json.dumps(alreadyExplored))
        isThumbOut = os.path.join(args.temp_dir,\
                'is-thumb-out-%d.json' % cnt)
        write_tranlator_cfg(translator_file, cfg['segments'], \
                already_file, \
                isThumbIn, \
                isThumbOut, \
                args.jump_table_file
                )
        log.debug("[translator] already explored %d (intervals)" % len(alreadyExplored))

        # run translator
        if cfg['endianness'] == 'little':
            pass
        else:
            #translator_path = translator_path_build_dir + \
            #        '/qemu-debug/armeb-s2e-softmmu/qemu-system-armeb'
            #raise Exception("N/A")
            pass
        ok = run_translator(args.temp_dir, machine_file, \
                translator_file, cnt)
        if ok is False:
            log.warning("(initial) crashed with entry: 0x%08x" % e)

        # run passes
        out_funcs = os.path.join(args.temp_dir, 'funcs-%d.bc' % cnt)
        out_remaining = os.path.join(args.temp_dir, 'remaining-%d.bc' % cnt)

        raw_llvm = os.path.join(args.temp_dir, 's2e-last', 'translated_bbs.bc')
        ok = run_passes_pre(raw_llvm, out_funcs, \
                out_remaining, cfg, args.jump_table_file)
        #cov.extend_with_bc(out_funcs)
        if ok is True:
            qemu_path_file = os.path.join(args.temp_dir, 'qemu-%d.log' % cnt)
            cov.extend_with_qemu_log(qemu_path_file)


        out_funcs_indirect = os.path.join(args.temp_dir, 'funcs-indirect-%d.bc' % cnt)
        out_remaining_indirect = os.path.join(args.temp_dir, 'remaining-indirect-%d.bc' % cnt)
        out_new_targets_funcs_json = os.path.join(args.temp_dir, 'new-targets-funcs-%d.json' % cnt)
        out_new_targets_remaining_json = os.path.join(args.temp_dir, 'new-targets-remaining-%d.json' % cnt)
        # run indirect solver

        ok = run_indirect_solver(out_funcs,\
                out_funcs_indirect, out_new_targets_funcs_json)
        new_discovered = get_new_discovered_from_json(out_new_targets_funcs_json)
        for b in new_discovered:
            if not cov.visited(b):
                entryQueue.append(b)

        if ok is False:
            log.warning("(passes) crashed with entry: 0x%08x" % e)
        else:
            #cov.extend_with_bc(out_remaining)
            func_bcs.append(out_funcs_indirect)
        func_bcs.append(out_funcs)

        run_indirect_solver(out_remaining,\
                out_remaining_indirect, out_new_targets_remaining_json)

        new_discovered = get_new_discovered_from_json(out_new_targets_remaining_json)
        for b in new_discovered:
            if not cov.visited(b):
                entryQueue.append(b)

        out_funcs_arm_thumb_json = os.path.join(args.temp_dir, \
                'arm-thumb-funcs-unmerged-%d.json' % cnt)
        run_arm_dump_thumb_bit(\
                out_funcs_indirect, out_funcs_arm_thumb_json)
        isThumbIn = os.path.join(args.temp_dir, \
                'arm-thumb-funcs-pcs-%d.json' % cnt)
        merge_arm_thumb_bit(isThumbIn, \
                isThumbOut, out_funcs_arm_thumb_json)
        # TODO: XXX: take into account the thumb bit
        # we don't really care if this operation succeeded

        # next state
        cnt += 1
        #isThumbIn = isThumbOut

    log.debug("[Translator] output folder is: %s" % args.temp_dir)

    if args.out is None:
        final_linked = os.path.join(args.temp_dir, 'final-linked.bc')
        args.out = os.path.join(args.temp_dir, 'final.bc')
    else:
        final_linked = os.path.join(args.temp_dir,
                os.path.basename(args.out)+'.linked.bc')


    sys.stdout.flush()


    ok = run_custom_linker(linker_path, func_bcs, final_linked)
    function_cnt = 0;
    if ok is False:
        log.warning("(linker) crashed with %s" % ' '.join(func_bcs))
    else:
        try:
            with open(final_linked + '.fcnt', 'rt') as f:
                function_cnt = int(f.read())
        except:
            function_cnt = -1
        log.debug("[Translator] final linked output is in %s" % \
                (final_linked))


    if ok:
        ok_passes_post = run_passes_post(final_linked, args.out+'post_passes.bc')
        if ok_passes_post:
            log.debug("[Translator] final output is in %s" % \
                    args.out)
        else:
            log.warning("[Translator] passes post failed")

    inline_linked = os.path.join(args.temp_dir, args.out)
    generate_helper_bc(args.temp_dir)
    ok = run_inline_mmu_helpers(args.temp_dir, \
            args.out+'post_passes.bc', inline_linked)
    if ok:
        log.info("FINAL output is in %s (%d functions)" % \
                (inline_linked, function_cnt))
    else:
        log.critical("[Translator] inline linked failed")

    enable_echo()
    remove_lock()

def remove_lock():
    if args.remove_lock:
        try:
            os.remove(os.path.join(args.temp_dir, 'lock'))
        except:
            pass

def enable_echo():
    try:
        import termios, sys
        fd = sys.stdin.fileno()
        old = termios.tcgetattr(fd)
        new = termios.tcgetattr(fd)
        new[3] = new[3] | termios.ECHO
        termios.tcsetattr(fd, termios.TCSADRAIN, new)
    except Exception:
        pass


if __name__ == "__main__":
    try:
        signal.signal(signal.SIGINT, signal_handler)
        print('Press Ctrl+C')
        main()
    except (Exception, KeyboardInterrupt) as e:
        #print("Got exception")
        if isinstance(e, KeyboardInterrupt):
            print("Got ^C, stopping, please wait!")
            should_continue = False
        else:
            print("%s" % traceback.format_exc())
        remove_lock()
