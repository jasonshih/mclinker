//===- GNUArchiveReader.cpp -----------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include <mcld/LD/GNUArchiveReader.h>

#include <mcld/Module.h>
#include <mcld/InputTree.h>
#include <mcld/MC/Attribute.h>
#include <mcld/MC/Input.h>
#include <mcld/LD/ResolveInfo.h>
#include <mcld/LD/ELFObjectReader.h>
#include <mcld/Support/FileSystem.h>
#include <mcld/Support/FileHandle.h>
#include <mcld/Support/MemoryArea.h>
#include <mcld/Support/MemoryRegion.h>
#include <mcld/Support/MsgHandling.h>
#include <mcld/Support/Path.h>
#include <mcld/ADT/SizeTraits.h>

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Host.h>

#include <cstring>
#include <cstdlib>

using namespace mcld;

GNUArchiveReader::GNUArchiveReader(Module& pModule,
                                   ELFObjectReader& pELFObjectReader)
 : m_Module(pModule),
   m_ELFObjectReader(pELFObjectReader)
{
}

GNUArchiveReader::~GNUArchiveReader()
{
}

/// isMyFormat
bool GNUArchiveReader::isMyFormat(Input& pInput, bool &pContinue) const
{
  assert(pInput.hasMemArea());
  if (pInput.memArea()->size() < Archive::MAGIC_LEN)
    return false;

  MemoryRegion* region = pInput.memArea()->request(pInput.fileOffset(),
                                                   Archive::MAGIC_LEN);
  const char* str = reinterpret_cast<const char*>(region->getBuffer());

  bool result = false;
  assert(NULL != str);
  pContinue = true;
  if (isArchive(str) || isThinArchive(str))
    result = true;

  pInput.memArea()->release(region);
  return result;
}

/// isArchive
bool GNUArchiveReader::isArchive(const char* pStr) const
{
  return (0 == memcmp(pStr, Archive::MAGIC, Archive::MAGIC_LEN));
}

/// isThinArchive
bool GNUArchiveReader::isThinArchive(const char* pStr) const
{
  return (0 == memcmp(pStr, Archive::THIN_MAGIC, Archive::MAGIC_LEN));
}

/// isThinArchive
bool GNUArchiveReader::isThinArchive(Input& pInput) const
{
  assert(pInput.hasMemArea());
  MemoryRegion* region = pInput.memArea()->request(pInput.fileOffset(),
                                                   Archive::MAGIC_LEN);
  const char* str = reinterpret_cast<const char*>(region->getBuffer());

  bool result = false;
  assert(NULL != str);
  if (isThinArchive(str))
    result = true;

  pInput.memArea()->release(region);
  return result;
}

bool GNUArchiveReader::readArchive(Archive& pArchive)
{
  // bypass the empty archive
  if (Archive::MAGIC_LEN == pArchive.getARFile().memArea()->handler()->size())
    return true;

  if (pArchive.getARFile().attribute()->isWholeArchive())
    return includeAllMembers(pArchive);

  // if this is the first time read this archive, setup symtab and strtab
  if (pArchive.getSymbolTable().empty()) {
  // read the symtab of the archive
  readSymbolTable(pArchive);

  // read the strtab of the archive
  readStringTable(pArchive);

  // add root archive to ArchiveMemberMap
  pArchive.addArchiveMember(pArchive.getARFile().name(),
                            pArchive.inputs().root(),
                            &InputTree::Downward);
  }

  // include the needed members in the archive and build up the input tree
  bool willSymResolved;
  do {
    willSymResolved = false;
    for (size_t idx = 0; idx < pArchive.numOfSymbols(); ++idx) {
      // bypass if we already decided to include this symbol or not
      if (Archive::Symbol::Unknown != pArchive.getSymbolStatus(idx))
        continue;

      // bypass if another symbol with the same object file offset is included
      if (pArchive.hasObjectMember(pArchive.getObjFileOffset(idx))) {
        pArchive.setSymbolStatus(idx, Archive::Symbol::Include);
        continue;
      }

      // check if we should include this defined symbol
      Archive::Symbol::Status status =
        shouldIncludeSymbol(pArchive.getSymbolName(idx));
      if (Archive::Symbol::Unknown != status)
        pArchive.setSymbolStatus(idx, status);

      if (Archive::Symbol::Include == status) {
        // include the object member from the given offset
        includeMember(pArchive, pArchive.getObjFileOffset(idx));
        willSymResolved = true;
      } // end of if
    } // end of for
  } while (willSymResolved);

  return true;
}

/// readMemberHeader - read the header of a member in a archive file and then
/// return the corresponding archive member (it may be an input object or
/// another archive)
/// @param pArchiveRoot  - the archive root that holds the strtab (extended
///                        name table)
/// @param pArchiveFile  - the archive that contains the needed object
/// @param pFileOffset   - file offset of the member header in the archive
/// @param pNestedOffset - used when we find a nested archive
/// @param pMemberSize   - the file size of this member
Input* GNUArchiveReader::readMemberHeader(Archive& pArchiveRoot,
                                          Input& pArchiveFile,
                                          uint32_t pFileOffset,
                                          uint32_t& pNestedOffset,
                                          size_t& pMemberSize)
{
  assert(pArchiveFile.hasMemArea());

  MemoryRegion* header_region =
    pArchiveFile.memArea()->request((pArchiveFile.fileOffset() + pFileOffset),
                                    sizeof(Archive::MemberHeader));
  const Archive::MemberHeader* header =
    reinterpret_cast<const Archive::MemberHeader*>(header_region->getBuffer());

  assert(0 == memcmp(header->fmag, Archive::MEMBER_MAGIC, sizeof(header->fmag)));

  pMemberSize = atoi(header->size);

  // parse the member name and nested offset if any
  std::string member_name;
  llvm::StringRef name_field(header->name, sizeof(header->name));
  if ('/' != header->name[0]) {
    // this is an object file in an archive
    size_t pos = name_field.find_first_of('/');
    member_name.assign(name_field.substr(0, pos).str());
  }
  else {
    // this is an object/archive file in a thin archive
    size_t begin = 1;
    size_t end = name_field.find_first_of(" :");
    uint32_t name_offset = 0;
    // parse the name offset
    name_field.substr(begin, end - begin).getAsInteger(10, name_offset);

    if (':' == name_field[end]) {
      // there is a nested offset
      begin = end + 1;
      end = name_field.find_first_of(' ', begin);
      name_field.substr(begin, end - begin).getAsInteger(10, pNestedOffset);
    }

    // get the member name from the extended name table
    assert(pArchiveRoot.hasStrTable());
    begin = name_offset;
    end = pArchiveRoot.getStrTable().find_first_of('\n', begin);
    member_name.assign(pArchiveRoot.getStrTable().substr(begin, end - begin -1));
  }

  Input* member = NULL;
  bool isThinAR = isThinArchive(pArchiveFile);
  if (!isThinAR) {
    // this is an object file in an archive
    member = pArchiveRoot.getMemberFile(pArchiveFile,
                                        isThinAR,
                                        member_name,
                                        pArchiveFile.path(),
                                        (pFileOffset +
                                         sizeof(Archive::MemberHeader)));
  }
  else {
    // this is a member in a thin archive
    // try to find if this is a archive already in the map first
    Archive::ArchiveMember* ar_member =
      pArchiveRoot.getArchiveMember(member_name);
    if (NULL != ar_member) {
      return ar_member->file;
    }

    // get nested file path, the nested file's member name is the relative
    // path to the archive containing it.
    sys::fs::Path input_path(pArchiveFile.path().parent_path());
    if (!input_path.empty())
      input_path.append(member_name);
    else
      input_path.assign(member_name);

    member = pArchiveRoot.getMemberFile(pArchiveFile,
                                        isThinAR,
                                        member_name,
                                        input_path);
  }

  pArchiveFile.memArea()->release(header_region);
  return member;
}

template <size_t SIZE>
static void readSymbolTableEntries(Archive& pArchive, MemoryRegion& pMemRegion)
{
  typedef typename SizeTraits<SIZE>::Offset Offset;

  const Offset* data = reinterpret_cast<const Offset*>(pMemRegion.getBuffer());

  // read the number of symbols
  Offset number = 0;
  if (llvm::sys::IsLittleEndianHost)
    number = mcld::bswap<SIZE>(*data);
  else
    number = *data;

  // set up the pointers for file offset and name offset
  ++data;
  const char* name = reinterpret_cast<const char*>(data + number);

  // add the archive symbols
  for (Offset i = 0; i < number; ++i) {
    if (llvm::sys::IsLittleEndianHost)
      pArchive.addSymbol(name, mcld::bswap<SIZE>(*data));
    else
      pArchive.addSymbol(name, *data);
    name += strlen(name) + 1;
    ++data;
  }
}

/// readSymbolTable - read the archive symbol map (armap)
bool GNUArchiveReader::readSymbolTable(Archive& pArchive)
{
  assert(pArchive.getARFile().hasMemArea());

  MemoryRegion* header_region =
    pArchive.getARFile().memArea()->request((pArchive.getARFile().fileOffset() +
                                             Archive::MAGIC_LEN),
                                            sizeof(Archive::MemberHeader));
  const Archive::MemberHeader* header =
    reinterpret_cast<const Archive::MemberHeader*>(header_region->getBuffer());
  assert(0 == memcmp(header->fmag, Archive::MEMBER_MAGIC, sizeof(header->fmag)));

  int symtab_size = atoi(header->size);
  pArchive.setSymTabSize(symtab_size);

  if (!pArchive.getARFile().attribute()->isWholeArchive()) {
    MemoryRegion* symtab_region =
      pArchive.getARFile().memArea()->request(
                                            (pArchive.getARFile().fileOffset() +
                                             Archive::MAGIC_LEN +
                                             sizeof(Archive::MemberHeader)),
                                            symtab_size);

    if (0 == strncmp(header->name, Archive::SVR4_SYMTAB_NAME,
                                   strlen(Archive::SVR4_SYMTAB_NAME)))
      readSymbolTableEntries<32>(pArchive, *symtab_region);
    else if (0 == strncmp(header->name, Archive::IRIX6_SYMTAB_NAME,
                                        strlen(Archive::IRIX6_SYMTAB_NAME)))
      readSymbolTableEntries<64>(pArchive, *symtab_region);
    else
      unreachable(diag::err_unsupported_archive);

    pArchive.getARFile().memArea()->release(symtab_region);
  }
  pArchive.getARFile().memArea()->release(header_region);
  return true;
}

/// readStringTable - read the strtab for long file name of the archive
bool GNUArchiveReader::readStringTable(Archive& pArchive)
{
  size_t offset = Archive::MAGIC_LEN +
                  sizeof(Archive::MemberHeader) +
                  pArchive.getSymTabSize();

  if (0x0 != (offset & 1))
    ++offset;

  assert(pArchive.getARFile().hasMemArea());

  MemoryRegion* header_region =
    pArchive.getARFile().memArea()->request((pArchive.getARFile().fileOffset() +
                                             offset),
                                            sizeof(Archive::MemberHeader));
  const Archive::MemberHeader* header =
    reinterpret_cast<const Archive::MemberHeader*>(header_region->getBuffer());

  assert(0 == memcmp(header->fmag, Archive::MEMBER_MAGIC, sizeof(header->fmag)));

  if (0 == memcmp(header->name, Archive::STRTAB_NAME, sizeof(header->name))) {
    // read the extended name table
    int strtab_size = atoi(header->size);
    MemoryRegion* strtab_region =
      pArchive.getARFile().memArea()->request(
                                   (pArchive.getARFile().fileOffset() +
                                    offset + sizeof(Archive::MemberHeader)),
                                   strtab_size);
    const char* strtab =
      reinterpret_cast<const char*>(strtab_region->getBuffer());
    pArchive.getStrTable().assign(strtab, strtab_size);
    pArchive.getARFile().memArea()->release(strtab_region);
  }
  pArchive.getARFile().memArea()->release(header_region);
  return true;
}

/// shouldIncludeStatus - given a sym name from armap and check if including
/// the corresponding archive member, and then return the decision
enum Archive::Symbol::Status
GNUArchiveReader::shouldIncludeSymbol(const llvm::StringRef& pSymName) const
{
  // TODO: handle symbol version issue and user defined symbols
  const ResolveInfo* info = m_Module.getNamePool().findInfo(pSymName);
  if (NULL != info) {
    if (!info->isUndef())
      return Archive::Symbol::Exclude;
    if (info->isWeak())
      return Archive::Symbol::Unknown;
    return Archive::Symbol::Include;
  }
  return Archive::Symbol::Unknown;
}

/// includeMember - include the object member in the given file offset, and
/// return the size of the object
/// @param pArchiveRoot - the archive root
/// @param pFileOffset  - file offset of the member header in the archive
size_t GNUArchiveReader::includeMember(Archive& pArchive, uint32_t pFileOffset)
{
  Input* cur_archive = &(pArchive.getARFile());
  Input* member = NULL;
  uint32_t file_offset = pFileOffset;
  size_t size = 0;
  do {
    uint32_t nested_offset = 0;
    // use the file offset in current archive to find out the member we
    // want to include
    member = readMemberHeader(pArchive,
                              *cur_archive,
                              file_offset,
                              nested_offset,
                              size);
    assert(member != NULL);
    // bypass if we get an archive that is already in the map
    if (Input::Archive == member->type()) {
        cur_archive = member;
        file_offset = nested_offset;
        continue;
    }

    // insert a node into the subtree of current archive.
    Archive::ArchiveMember* parent =
      pArchive.getArchiveMember(cur_archive->name());

    assert(NULL != parent);
    pArchive.inputs().insert(parent->lastPos, *(parent->move), *member);

    // move the iterator to new created node, and also adjust the
    // direction to Afterward for next insertion in this subtree
    parent->move->move(parent->lastPos);
    parent->move = &InputTree::Afterward;
    bool doContinue = false;

    if (m_ELFObjectReader.isMyFormat(*member, doContinue)) {
      member->setType(Input::Object);
      pArchive.addObjectMember(pFileOffset, parent->lastPos);
      m_ELFObjectReader.readHeader(*member);
      m_ELFObjectReader.readSections(*member);
      m_ELFObjectReader.readSymbols(*member);
      m_Module.getObjectList().push_back(member);
    }
    else if (doContinue && isMyFormat(*member, doContinue)) {
      member->setType(Input::Archive);
      // when adding a new archive node, set the iterator to archive
      // itself, and set the direction to Downward
      pArchive.addArchiveMember(member->name(),
                                parent->lastPos,
                                &InputTree::Downward);
      cur_archive = member;
      file_offset = nested_offset;
    }
  } while (Input::Object != member->type());
  return size;
}

/// includeAllMembers - include all object members. This is called if
/// --whole-archive is the attribute for this archive file.
bool GNUArchiveReader::includeAllMembers(Archive& pArchive)
{
  // read the symtab of the archive
  readSymbolTable(pArchive);

  // read the strtab of the archive
  readStringTable(pArchive);

  // add root archive to ArchiveMemberMap
  pArchive.addArchiveMember(pArchive.getARFile().name(),
                            pArchive.inputs().root(),
                            &InputTree::Downward);

  bool isThinAR = isThinArchive(pArchive.getARFile());
  uint32_t begin_offset = pArchive.getARFile().fileOffset() +
                          Archive::MAGIC_LEN +
                          sizeof(Archive::MemberHeader) +
                          pArchive.getSymTabSize();
  if (pArchive.hasStrTable()) {
    if (0x0 != (begin_offset & 1))
      ++begin_offset;
    begin_offset += sizeof(Archive::MemberHeader) +
                    pArchive.getStrTable().size();
  }
  uint32_t end_offset = pArchive.getARFile().memArea()->handler()->size();
  for (uint32_t offset = begin_offset;
       offset < end_offset;
       offset += sizeof(Archive::MemberHeader)) {

    size_t size = includeMember(pArchive, offset);

    if (!isThinAR) {
      offset += size;
    }

    if (0x0 != (offset & 1))
      ++offset;
  }
  return true;
}

