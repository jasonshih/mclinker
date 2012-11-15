//===- Linker.h -----------------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef MCLD_LINKER_H
#define MCLD_LINKER_H
#ifdef ENABLE_UNITTEST
#include <gtest.h>
#endif

#include <string>

namespace mcld {

class Module;
class LinkerConfig;

class Target;
class TargetLDBackend;
class DiagnosticPrinter;

class IRBuilder;
class ObjectLinker;

class FileHandle;
class MemoryArea;

/** \class Linker
*  \brief Linker is a modular linker.
*/
class Linker
{
public:
  Linker();

  ~Linker();

  bool config(const LinkerConfig& pConfig, IRBuilder& pBuilder);

  bool link(Module& pModule);

  bool emit(MemoryArea& pOutput);

  bool emit(const std::string& pPath);

  bool emit(int pFileDescriptor);

  bool reset();

  bool check();

private:
  bool initDiagnosticEngine();

  bool initTarget();

  bool initBackend();

private:
  const LinkerConfig* m_pConfig;
  Module* m_pModule;
  IRBuilder* m_pIRBuilder;

  const Target* m_pTarget;
  TargetLDBackend* m_pBackend;
  DiagnosticPrinter* m_pPrinter;
  ObjectLinker* m_pObjLinker;
};

} // namespace of MC Linker

#endif

