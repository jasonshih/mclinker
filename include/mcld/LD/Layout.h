//===- Layout.h -----------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef MCLD_LAYOUT_H
#define MCLD_LAYOUT_H
#ifdef ENABLE_UNITTEST
#include <gtest.h>
#endif

namespace mcld
{

/** \class Layout
 *  \brief Layout records the order and offset of all regions
 */
class Layout
{
public:
  typedef std::vector<llvm::MCSectionData*> SectionOrder;
  typedef SectionOrder::iterator sect_iterator;
  typedef SectionOrder::const_iterator const_sect_iterator;

public:
  Layout();
  ~Layout();

  /// layoutFragment - perform layout for single fragment
  /// Assuming the previous fragment has already been laid out correctly.
  /// @return the offset of the file to laid out the fragment
  uint64_t layoutFragment(MCFragment& pFrag);

  // -----  modifiers  ----- //
  bool layout(MCLinker& pLinker, LDFileFormat& pFormat);

  // -----  iterators  ----- //
  sect_iterator sect_begin()
  { return m_SectionOrder.begin(); }

  sect_iterator sect_end()
  { return m_SectionOrder.end(); }

  const_sect_iterator sect_begin() const
  { return m_SectionOrder.begin(); }

  const_sect_iterator sect_end() const
  { return m_SectionOrder.end(); }

private:
  /// a vector to describe the order of sections
  SectionOrder m_SectionOrder;
};

} // namespace of mcld

#endif

