//===- LinkerConfig.cpp ---------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "alone/Support/LinkerConfig.h"
#include "alone/Support/Log.h"

#include <llvm/Support/Signals.h>

#include <mcld/LinkerConfig.h>
#include <mcld/MC/MCLDDirectory.h>
#include <mcld/MC/ZOption.h>
#include <mcld/LD/TextDiagnosticPrinter.h>
#include <mcld/Support/Path.h>
#include <mcld/Support/MsgHandling.h>
#include <mcld/Support/raw_ostream.h>

using namespace alone;

LinkerConfig::LinkerConfig(const std::string &pTriple)
  : mTriple(pTriple), mSOName(), mTarget(NULL), mLDConfig(NULL),
    mDiagLineInfo(NULL), mDiagPrinter(NULL) {

  initializeTarget();
  initializeLDInfo();
  initializeDiagnostic();
}

LinkerConfig::~LinkerConfig() {
  delete mLDConfig;

  if (mDiagPrinter->getNumErrors() != 0) {
    // If here, the program failed ungracefully. Run the interrupt handlers to
    // ensure any other cleanups (e.g., files that registered by
    // RemoveFileOnSignal(...)) getting done before exit.
    llvm::sys::RunInterruptHandlers();
  }
  mDiagPrinter->finish();

  delete mDiagLineInfo;
  delete mDiagPrinter;
}

bool LinkerConfig::initializeTarget() {
  std::string error;
  mTarget = mcld::TargetRegistry::lookupTarget(mTriple, error);
  if (NULL != mTarget) {
    return true;
  } else {
    ALOGE("Cannot initialize mcld::Target for given triple '%s'! (%s)\n",
          mTriple.c_str(), error.c_str());
    return false;
  }
}

bool LinkerConfig::initializeLDInfo() {
  if (NULL != mLDConfig) {
    ALOGE("Cannot initialize mcld::MCLDInfo for given triple '%s!\n",
          mTriple.c_str());
    return false;
  }

  mLDConfig = new mcld::LinkerConfig(getTriple());
  mLDConfig->setCodeGenType(mcld::LinkerConfig::Exec);
  return true;
}

bool LinkerConfig::initializeDiagnostic() {
  // Set up MsgHandler.
  mDiagLineInfo = mTarget->createDiagnosticLineInfo(*mTarget, mTriple);

  mDiagPrinter = new mcld::TextDiagnosticPrinter(mcld::errs(), *mLDConfig);

  mcld::InitializeDiagnosticEngine(*mLDConfig, mDiagLineInfo, mDiagPrinter);

  return true;
}

bool LinkerConfig::isShared() const {
  return (mcld::LinkerConfig::DynObj == mLDConfig->codeGenType());
}

void LinkerConfig::setShared(bool pEnable) {
  if (pEnable)
    mLDConfig->setCodeGenType(mcld::LinkerConfig::DynObj);
  else
    mLDConfig->setCodeGenType(mcld::LinkerConfig::Exec);
  return;
}

void LinkerConfig::setBsymbolic(bool pEnable) {
  mLDConfig->options().setBsymbolic(pEnable);
  return;
}

void LinkerConfig::setSOName(const std::string &pSOName) {
  mLDConfig->options().setSOName(pSOName);
  return;
}

void LinkerConfig::setDyld(const std::string &pDyld) {
  mLDConfig->options().setDyld(pDyld);
  return;
}

void LinkerConfig::setSysRoot(const std::string &pSysRoot) {
  mLDConfig->options().setSysroot(mcld::sys::fs::Path(pSysRoot));
  return;
}

void LinkerConfig::setZOption(unsigned int pOptions) {
  mcld::ZOption option;
  if (pOptions & kCombReloc) {
    option.setKind(mcld::ZOption::CombReloc);
    mLDConfig->options().addZOption(option);
  }
  else {
    option.setKind(mcld::ZOption::NoCombReloc);
    mLDConfig->options().addZOption(option);
  }

  if (pOptions & kDefs) {
    option.setKind(mcld::ZOption::Defs);
    mLDConfig->options().addZOption(option);
  }

  if (pOptions & kExecStack) {
    option.setKind(mcld::ZOption::ExecStack);
    mLDConfig->options().addZOption(option);
  }
  else {
    option.setKind(mcld::ZOption::NoExecStack);
    mLDConfig->options().addZOption(option);
  }

  if (pOptions & kInitFirst) {
    option.setKind(mcld::ZOption::InitFirst);
    mLDConfig->options().addZOption(option);
  }

  if (pOptions & kInterPose) {
    option.setKind(mcld::ZOption::InterPose);
    mLDConfig->options().addZOption(option);
  }

  if (pOptions & kLoadFltr) {
    option.setKind(mcld::ZOption::LoadFltr);
    mLDConfig->options().addZOption(option);
  }

  if (pOptions & kMulDefs) {
    option.setKind(mcld::ZOption::MulDefs);
    mLDConfig->options().addZOption(option);
  }

  if (pOptions & kNoCopyReloc) {
    option.setKind(mcld::ZOption::NoCopyReloc);
    mLDConfig->options().addZOption(option);
  }

  if (pOptions & kNoDefaultLib) {
    option.setKind(mcld::ZOption::NoDefaultLib);
    mLDConfig->options().addZOption(option);
  }

  if (pOptions & kNoDelete) {
    option.setKind(mcld::ZOption::NoDelete);
    mLDConfig->options().addZOption(option);
  }

  if (pOptions & kNoDLOpen) {
    option.setKind(mcld::ZOption::NoDLOpen);
    mLDConfig->options().addZOption(option);
  }

  if (pOptions & kNoDump) {
    option.setKind(mcld::ZOption::NoDump);
    mLDConfig->options().addZOption(option);
  }

  if (pOptions & kRelro) {
    option.setKind(mcld::ZOption::Relro);
    mLDConfig->options().addZOption(option);
  }
  else {
    option.setKind(mcld::ZOption::NoRelro);
    mLDConfig->options().addZOption(option);
  }

  if (pOptions & kLazy) {
    option.setKind(mcld::ZOption::Lazy);
    mLDConfig->options().addZOption(option);
  }
  else {
    option.setKind(mcld::ZOption::Now);
    mLDConfig->options().addZOption(option);
  }

  if (pOptions & kOrigin) {
    option.setKind(mcld::ZOption::Origin);
    mLDConfig->options().addZOption(option);
  }
}

void LinkerConfig::addWrap(const std::string &pWrapSymbol) {
  bool exist = false;

  // Add wname -> __wrap_wname.
  mcld::StringEntry<llvm::StringRef>* to_wrap =
               mLDConfig->scripts().renameMap().insert(pWrapSymbol, exist);

  std::string to_wrap_str = "__wrap_" + pWrapSymbol;
  to_wrap->setValue(to_wrap_str);

  if (exist) {
    mcld::warning(mcld::diag::rewrap) << pWrapSymbol << to_wrap_str;
  }

  // Add __real_wname -> wname.
  std::string from_real_str = "__real_" + pWrapSymbol;
  mcld::StringEntry<llvm::StringRef>* from_real =
             mLDConfig->scripts().renameMap().insert(from_real_str, exist);
  from_real->setValue(pWrapSymbol);

  if (exist) {
    mcld::warning(mcld::diag::rewrap) << pWrapSymbol << from_real_str;
  }

  return;
}

void LinkerConfig::addPortable(const std::string &pPortableSymbol) {
  bool exist = false;

  // Add pname -> pname_portable.
  mcld::StringEntry<llvm::StringRef>* to_port =
                mLDConfig->scripts().renameMap().insert(pPortableSymbol, exist);

  std::string to_port_str = pPortableSymbol + "_portable";
  to_port->setValue(to_port_str);

  if (exist) {
    mcld::warning(mcld::diag::rewrap) << pPortableSymbol << to_port_str;
}

  // Add __real_pname -> pname.
  std::string from_real_str = "__real_" + pPortableSymbol;
  mcld::StringEntry<llvm::StringRef>* from_real =
           mLDConfig->scripts().renameMap().insert(from_real_str, exist);

  from_real->setValue(pPortableSymbol);

  if (exist) {
    mcld::warning(mcld::diag::rewrap) << pPortableSymbol << from_real_str;
  }

  return;
}

void LinkerConfig::addSearchDir(const std::string &pDirPath) {
  // SearchDirs will remove the created MCLDDirectory.
  if (!mLDConfig->options().directories().insert(pDirPath)) {
    mcld::warning(mcld::diag::warn_cannot_open_search_dir) << pDirPath;
  }
}
