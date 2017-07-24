#!/usr/bin/env python
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

import os

class TranslatorPaths(object):
    def __init__(self, prefix_path=''):
        self.prefix_path = prefix_path

    def get(self, s=''):
        return os.path.join(self.prefix_path, s)

    def get_bin(self, s=''):
        return self.get(os.path.join('bin', s))

    def get_lib(self, s=''):
        return self.get(os.path.join('lib', s))

    def get_qemu(self):
        return self.get_bin(os.path.join('qemu-release', 'arm-s2e-softmmu', 'qemu-system-arm'))

    def get_linker(self):
        return self.get_bin('linky')

    def get_translator_so(self):
        return self.get_lib('translator.so')

    def get_llvm_opt(self):
        return self.get_bin('opt')

    def get_llvm_as(self):
        return self.get_bin('llvm-as')

    def get_llvm_link(self):
        return self.get_bin('llvm-link')

    def get_ll_helpers(self):
        return self.get_lib('mem-ops-alt.ll')

