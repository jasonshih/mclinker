//===- X86Emulation.cpp ---------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "X86.h"
#include <mcld/LinkerConfig.h>
#include <mcld/Target/ELFEmulation.h>
#include <mcld/Support/TargetRegistry.h>

namespace mcld {

static bool MCLDEmulateX86ELF(LinkerConfig& pConfig)
{
  if (!MCLDEmulateELF(pConfig))
    return false;

  // set up bitclass and endian
  pConfig.targets().setEndian(TargetOptions::Little);
  unsigned int bitclass;
  Triple::ArchType arch = pConfig.targets().triple().getArch();
  if (arch == Triple::x86) {
    bitclass = 32;
  }
  else {
    assert (arch == Triple::x86_64);
    bitclass = 64;
  }
  pConfig.targets().setBitClass(bitclass);

  // set up target-dependent constraints of attributes
  pConfig.attribute().constraint().enableWholeArchive();
  pConfig.attribute().constraint().enableAsNeeded();
  pConfig.attribute().constraint().setSharedSystem();

  // set up the predefined attributes
  pConfig.attribute().predefined().unsetWholeArchive();
  pConfig.attribute().predefined().unsetAsNeeded();
  pConfig.attribute().predefined().setDynamic();
  return true;
}

//===----------------------------------------------------------------------===//
// emulateX86LD - the help function to emulate X86 ld
//===----------------------------------------------------------------------===//
bool emulateX86LD(const std::string& pTriple, LinkerConfig& pConfig)
{
  llvm::Triple theTriple(pTriple);
  if (theTriple.isOSDarwin()) {
    assert(0 && "MachO linker has not supported yet");
    return false;
  }
  if (theTriple.isOSWindows()) {
    assert(0 && "COFF linker has not supported yet");
    return false;
  }

  return MCLDEmulateX86ELF(pConfig);
}

} // namespace of mcld

//===----------------------------------------------------------------------===//
// X86Emulation
//===----------------------------------------------------------------------===//
extern "C" void MCLDInitializeX86Emulation() {
  // Register the emulation
  mcld::TargetRegistry::RegisterEmulation(mcld::TheX86_32Target, mcld::emulateX86LD);
  mcld::TargetRegistry::RegisterEmulation(mcld::TheX86_64Target, mcld::emulateX86LD);
}

