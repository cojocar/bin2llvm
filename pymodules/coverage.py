#!/usr/bin/evn python
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

from llvm.core import Module
import sys
from llvm import *
from llvm.core import *
import pickle
import sys
import logging
log = logging.getLogger(__file__)
import re
import json
import argparse

def cg_load_bitcode_from_file(file_name):
    try:
        with open(file_name, "rb") as fin:
            mod = Module.from_bitcode(fin)
    except:
        log.debug("Failed to load bitcode: " + file_name)
        return None
    log.debug("Loaded bitcode from file: " + file_name)
    return mod

class CoverageStatus:
    def __init__(self):
        # visited intervals
        # we should use an interval tree here
        self._intervals = []
        self._visitedPCs = []
        pass

    def __get_start_and_end_of_a_func(self, func):
        maxPC = None
        minPC = None
        for b in func.basic_blocks:
            for i in b.instructions:
                m = i.get_metadata("INS_currPC")
                if m is None:
                    continue
                pc = int(str(m.getOperand(0))[len('metadata !"'):-1], 16)
                if pc > maxPC or maxPC is None:
                    maxPC = pc
                if pc < minPC or minPC is None:
                    minPC = pc
        return (minPC, maxPC)

    def extend_with_bc(self, bc_file):
        mod = cg_load_bitcode_from_file(bc_file)
        if mod is None:
            return

        for f in mod.functions:
            minPC, maxPC = self.__get_start_and_end_of_a_func(f)
            if minPC is None or maxPC is None:
                #print("skip func: %s" % f.name)
                pass
            else:
                #print("func: %s\t%s-%s" % (f.name, hex(minPC), hex(maxPC)))
                self._intervals.append((minPC, maxPC))
                try:
                    self._visitedPCs += range(minPC, maxPC)
                except MemoryError:
                    # ignore memory error
                    pass

    def extend_with_qemu(self, qemu_log):
        pass


    def visited(self, pc):
        for minPC, maxPC in self._intervals:
            if pc >= minPC and pc < maxPC:
                return True
        return False


    def getAlreadyExplored(self):
        return self._visitedPCs

class CoverageStatusQemu(object):
    def __init__(self):
        self._pc_re = re.compile(r'^(0x[0-9a-fA-F]{8})')
        # sorted list of intervals (sorted by the start address)
        self._intervals = []

    def _get_intervals_from_qemu(self, qemu_log):
        with open(qemu_log, 'rt') as f:
            data = f.read()
        intervals = []
        pc_prev = None
        pc_val = None
        for line in data.split('\n'):
            m = re.match(self._pc_re, line)
            if m is None:
                continue
            pc_val = int(m.group(0), 16)
            #print(hex(pc_val))

            if pc_prev is None:
                # first pc (and interval)
                pc_prev = pc_val
                interval_start = pc_prev
                continue

            # ARM/thumb specific
            if pc_val == pc_prev+4 or pc_val == pc_prev+2 or pc_val == pc_prev:
                # this is just an extension of the preivous interval
                pc_prev = pc_val
                #print("expand: %s -> %s" % (hex(interval_start), hex(pc_val)))
                continue

            #print("new interval %s->%s" % (hex(interval_start), hex(pc_prev)))
            # this is a new interval
            intervals.append((interval_start, pc_prev))

            interval_start = pc_val
            pc_prev = pc_val

        # last interval
        if pc_val is not None:
            intervals.append((interval_start, pc_val))

        # keep the meaningful intervals
        intervals = filter(lambda i : i[0] <= i[1], intervals)
        return sorted(intervals, cmp=lambda a, b: a[0] - b[0])


    def _merge_sorted_lists(self, a, b):
        ret = []
        while len(a) and len(b):
            if a[0] < b[0]:
                ret.append(a.pop(0))
            else:
                ret.append(b.pop(0))
        return ret + a + b

    #def _prune_merge_interval(self):
    #    # if we have two adjacdnt intervals merge them in one
    #    ret = []
    #    for idx in range(len(self._intervals)-1):
    #        ret.append()

    def extend_with_qemu_log(self, qemu_log):
        r = self._get_intervals_from_qemu(qemu_log)
        self._intervals = self._merge_sorted_lists(self._intervals, r)
        #print(json.dumps(self._intervals, indent=1))

    def visited(self, pc):
        for minPC, maxPC in self._intervals:
            if pc >= minPC and pc <= maxPC:
                return True
        return False

    def get_already_explored_intervals(self):
        return self._intervals

def main_bc():
    parser = argparse.ArgumentParser("Build the dynamic call graph and save it to disk")
    parser.add_argument("--bc", metavar="BC", \
            help="Use this LLVM as entry", \
            required=True)
    args = parser.parse_args()

    mod = cg_load_bitcode_from_file(args.bc)

    for f in mod.functions:
        minPC, maxPC = get_start_and_end_of_a_func(f)
        if minPC is None or maxPC is None:
            print("skip func: %s" % f.name)
        else:
            print("func: %s\t%s-%s" % (f.name, hex(minPC), hex(maxPC)))

def main_qemu():
    parser = argparse.ArgumentParser("Build the coverage of a qemu log")
    parser.add_argument("--qemu-log", metavar="LOG", \
            help="Use this QEMU log", \
            required=True)
    args = parser.parse_args()

    c = CoverageStatusQemu()
    c.extend_with_qemu_log(args.qemu_log)

if __name__ == "__main__":
    main_qemu()
