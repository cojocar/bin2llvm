/* vim: set expandtab ts=4 sw=4: */

/*
 * Copyright 2017 The bin2llvm Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __S2ETRASNFORM_PASS_H__
#define __S2ETRASNFORM_PASS_H__

#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Module.h"

#include <list>
#include <map>

struct S2ETransformPass : public llvm::FunctionPass {
public:
  static char ID;
  S2ETransformPass() : llvm::FunctionPass(ID) {initRegisterNames();}

  virtual bool runOnFunction(llvm::Function &F);

  struct offset_name {
      int offset; const char *name;
  };

private:
  llvm::Module *M;
  std::list<llvm::Instruction*> eraseList;
  std::list<llvm::Instruction*> eraseStoreList;
  std::list<llvm::Instruction*> eraseLoadList;
  std::map<llvm::Instruction*, llvm::Instruction*> replaceMap;
  std::map<unsigned int, std::string> nameMap;
  bool processArgUse(llvm::Value* argUse);
  bool processEnvUse(llvm::Value *envPointer, llvm::Value* envUse, unsigned int currentOffset);
  std::string getGlobalName(unsigned int offset);

  void initRegisterNames();
  void initRegisterNames(struct offset_name *, size_t count);
  void setRegisterName(unsigned int offset, std::string name);
  int fixupStores(llvm::Function &F);
  int fixupLoads(llvm::Function &F);
  int fixupEnvStruct(llvm::Function &F);
  void cleanup();
};

#endif
