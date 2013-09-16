//===- lib/Linker/Linker.cpp - Basic Linker functionality  ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains basic Linker functionality that all usages will need.
//
//===----------------------------------------------------------------------===//

#include "llvm/Linker.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/system_error.h"
using namespace llvm;

Linker::Linker(StringRef progname, StringRef modname,
               LLVMContext& C, unsigned flags):
  Context(C),
  Composite(new Module(modname, C)),
  LibPaths(),
  Flags(flags),
  Error(),
  ProgramName(progname) { }

Linker::Linker(StringRef progname, Module* aModule, unsigned flags) :
  Context(aModule->getContext()),
  Composite(aModule),
  LibPaths(),
  Flags(flags),
  Error(),
  ProgramName(progname) { }

Linker::~Linker() {
  delete Composite;
}

bool
Linker::error(StringRef message) {
  Error = message;
  if (!(Flags&QuietErrors))
    errs() << ProgramName << ": error: " << message << "\n";
  return true;
}

bool
Linker::warning(StringRef message) {
  Error = message;
  if (!(Flags&QuietWarnings))
    errs() << ProgramName << ": warning: " << message << "\n";
  return false;
}

void
Linker::verbose(StringRef message) {
  if (Flags&Verbose)
    errs() << "  " << message << "\n";
}

void
Linker::addPath(const sys::Path& path) {
  LibPaths.push_back(path);
}

void
Linker::addPaths(const std::vector<std::string>& paths) {
  for (unsigned i = 0, e = paths.size(); i != e; ++i)
    LibPaths.push_back(sys::Path(paths[i]));
}

void
Linker::addSystemPaths() {
  sys::Path::GetBitcodeLibraryPaths(LibPaths);
  LibPaths.insert(LibPaths.begin(),sys::Path("./"));
}

Module*
Linker::releaseModule() {
  Module* result = Composite;
  LibPaths.clear();
  Error.clear();
  Composite = 0;
  Flags = 0;
  return result;
}
