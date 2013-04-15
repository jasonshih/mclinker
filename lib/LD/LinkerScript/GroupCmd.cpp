//===- GroupCmd.cpp -------------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include <mcld/LD/LinkerScript/GroupCmd.h>
#include <mcld/MC/InputBuilder.h>
#include <mcld/MC/Attribute.h>
#include <mcld/Support/raw_ostream.h>
#include <mcld/Support/MsgHandling.h>
#include <mcld/InputTree.h>
#include <mcld/LD/GroupReader.h>

using namespace mcld;

//===----------------------------------------------------------------------===//
// GroupCmd
//===----------------------------------------------------------------------===//
GroupCmd::GroupCmd(InputTree& pInputTree,
                   InputBuilder& pBuilder,
                   GroupReader& pGroupReader,
                   const LinkerConfig& pConfig)
  : ScriptCommand(ScriptCommand::Group),
    m_InputTree(pInputTree),
    m_Builder(pBuilder),
    m_GroupReader(pGroupReader),
    m_Config(pConfig)
{
}

GroupCmd::~GroupCmd()
{
}

void GroupCmd::dump() const
{
  mcld::outs() << "GROUP ( ";
  bool prev = false, cur = false;
  for (ScriptInput::const_iterator it = scriptInput().begin(),
    ie = scriptInput().end(); it != ie; ++it) {
    cur = (*it).asNeeded();
    if (!prev && cur)
      mcld::outs() << "AS_NEEDED ( ";
    if (prev && !cur)
      mcld::outs() << " )";
    mcld::outs() << (*it).path().native() << " ";
    prev = cur;
  }
  if (!scriptInput().empty() && scriptInput().back().asNeeded())
    mcld::outs() << " )";
  mcld::outs() << " )\n";
}

void GroupCmd::activate()
{
  // TODO
}

