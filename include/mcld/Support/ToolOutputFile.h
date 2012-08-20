//===- ToolOutputFile.h ---------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef MCLD_SUPPORT_TOOL_OUTPUT_FILE_H
#define MCLD_SUPPORT_TOOL_OUTPUT_FILE_H
#ifdef ENABLE_UNITTEST
#include <gtest.h>
#endif

#include <string>
#include <mcld/Support/FileHandle.h>

namespace mcld {

class Path;
class FileHandle;
class MemoryArea;
class raw_mem_ostream;

/** \class ToolOutputFile
 *  \brief ToolOutputFile contains a raw_mem_ostream and adds extra new
 *  features:
 *   - The file is automatically deleted if the process is killed.
 *   - The file is automatically deleted when the TooOutputFile object is
 *     destoryed unless the client calls keep().
 */
class ToolOutputFile
{
public:
  ToolOutputFile(const sys::fs::Path& pPath,
                 FileHandle::OpenMode pMode,
                 FileHandle::Permission pPermission);

  ~ToolOutputFile();

  /// os - Return the contained raw_mem_ostream.
  raw_mem_ostream &os();

  /// memory - Return the contained MemoryArea.
  MemoryArea& memory();

  /// keep - Indicate that the tool's job wrt this output file has been
  /// successful and the file should not be deleted.
  void keep();

private:
  class CleanupInstaller
  {
  public:
    explicit CleanupInstaller(const std::string& pFilename);

    ~CleanupInstaller();

    /// Keep - The flag which indicates whether we should not delete the file.
    bool Keep;

  private:
    std::string m_Filename;
  }; 

private:
  FileHandle m_FileHandle;
  CleanupInstaller m_Installer;
  MemoryArea* m_pMemoryArea;
  raw_mem_ostream* m_pOStream;

};

} // namespace of mcld

#endif

