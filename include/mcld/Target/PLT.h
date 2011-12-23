//===- PLT.h --------------------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef PROCEDURE_LINKAGE_TABLE_H
#define PROCEDURE_LINKAGE_TABLE_H
#ifdef ENABLE_UNITTEST
#include <gtest.h>
#endif

#include "mcld/LD/LDSection.h"
#include <llvm/MC/MCAssembler.h>
#include <llvm/ADT/ilist.h>

namespace mcld
{

class ResolveInfo;

/** \class PLTEntry
 */
class PLTEntry : public llvm::MCFragment
{
public:
  PLTEntry(unsigned int size, unsigned char* content);
  virtual ~PLTEntry();

  unsigned int getEntrySize() const {
    return m_EntrySize;
  }
  unsigned char* getContent() const {
    return m_pContent;
  }

protected:
  virtual void initPLTEntry() = 0;

protected:
  unsigned int m_EntrySize;
  unsigned char* m_pContent;
};

/** \class PLT
 *  \brief Procedure linkage table
 */
class PLT
{
public:
  PLT(llvm::MCSectionData* pSectionData);
  virtual ~PLT();

  llvm::MCSectionData* getSectionData() {
    return m_pSectionData;
  }

public:
  /// reserveEntry - reseve the number of pNum of empty entries
  /// The empty entris are reserved for layout to adjust the fragment offset.
  virtual void reserveEntry(int pNum = 1) = 0;

  /// getEntry - get an empty entry or an exitsted filled entry with pSymbol.
  /// @param pSymbol - the target symbol
  /// @param pExist - ture if the a filled entry with pSymbol existed, otherwise false.
  virtual PLTEntry* getEntry(const ResolveInfo& pSymbol, bool& pExist) = 0;

protected:
  llvm::MCSectionData* m_pSectionData;
};

} // namespace of mcld

#endif
