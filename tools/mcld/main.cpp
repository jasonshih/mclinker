//===- mcld.cpp -----------------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include <stdlib.h>
#include <string>

#include <llvm/ADT/SmallString.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/system_error.h>

#include <alone/Config/Config.h>
#include <alone/Support/LinkerConfig.h>
#include <alone/Support/Initialization.h>
#include <alone/Support/TargetLinkerConfigs.h>
#include <alone/Linker.h>

using namespace alone;

//===----------------------------------------------------------------------===//
// Compiler Options
//===----------------------------------------------------------------------===//
#ifdef TARGET_BUILD
const std::string OptTargetTripe(DEFAULT_TARGET_TRIPLE_STRING);
#else
llvm::cl::opt<std::string>
OptTargetTriple("mtriple",
               llvm::cl::desc("Specify the target triple (default: "
                              DEFAULT_TARGET_TRIPLE_STRING ")"),
               llvm::cl::init(DEFAULT_TARGET_TRIPLE_STRING),
               llvm::cl::value_desc("triple"));

llvm::cl::alias OptTargetTripleC("C", llvm::cl::NotHidden,
                                llvm::cl::desc("Alias for -mtriple"),
                                llvm::cl::aliasopt(OptTargetTriple));
#endif

//===----------------------------------------------------------------------===//
// Command Line Options
// There are four kinds of command line options:
//   1. input, (may be a file, such as -m and /tmp/XXXX.o.)
//   2. scripting options, (represent a subset of link scripting language, such
//      as --defsym.)
//   3. and general options. (the rest of options)
//===----------------------------------------------------------------------===//
// General Options
//===----------------------------------------------------------------------===//
static llvm::cl::opt<std::string>
OptOutputFilename("o",
                  llvm::cl::desc("Output filename"),
                  llvm::cl::value_desc("filename"));

static llvm::cl::list<std::string>
OptSearchDirList("L",
                 llvm::cl::ZeroOrMore,
                 llvm::cl::desc("Add path searchdir to the list of paths that ld will "
                          "search for archive libraries and ld control scripts."),
                 llvm::cl::value_desc("searchdir"),
                 llvm::cl::Prefix);

static llvm::cl::opt<std::string>
OptSOName("soname",
          llvm::cl::desc("Set internal name of shared library"),
          llvm::cl::value_desc("name"));


static llvm::cl::opt<bool>
OptShared("shared",
          llvm::cl::desc("Create a shared library."),
          llvm::cl::init(false));

static llvm::cl::opt<std::string>
OptDyld("dynamic-linker",
        llvm::cl::desc("Set the name of the dynamic linker."),
        llvm::cl::value_desc("Program"));

//===----------------------------------------------------------------------===//
// Inputs
//===----------------------------------------------------------------------===//
static llvm::cl::list<std::string>
OptInputObjectFiles(llvm::cl::Positional,
                    llvm::cl::desc("[input object files]"),
                    llvm::cl::ZeroOrMore);

static llvm::cl::list<std::string>
OptNameSpecList("l",
                llvm::cl::ZeroOrMore,
                llvm::cl::desc("Add the archive or object file specified by "
                               "namespec to the list of files to link."),
                llvm::cl::value_desc("namespec"),
                llvm::cl::Prefix);

//===----------------------------------------------------------------------===//
// Scripting Options
//===----------------------------------------------------------------------===//
static llvm::cl::list<std::string>
OptWrapList("wrap",
            llvm::cl::ZeroOrMore,
            llvm::cl::desc("Use a wrap function fo symbol."),
            llvm::cl::value_desc("symbol"));

static llvm::cl::list<std::string>
OptPortList("portable",
            llvm::cl::ZeroOrMore,
            llvm::cl::desc("Use a portable function fo symbol."),
            llvm::cl::value_desc("symbol"));

//===----------------------------------------------------------------------===//
// Helper Functions
//===----------------------------------------------------------------------===//
// Override "mcld -version"
void MCLDVersionPrinter() {
  llvm::raw_ostream &os = llvm::outs();
  os << "mcld (The MCLinker Project, http://mclinker.googlecode.com/):\n"
     << "  Default target: " << DEFAULT_TARGET_TRIPLE_STRING << "\n";

  os << "\n";

  os << "LLVM (http://llvm.org/):\n";
  return;
}

static inline
bool ConfigLinker(Linker &pLinker)
{
  LinkerConfig* config = NULL;

#ifdef TARGET_BUILD
  config = new (std::nothrow) DefaultLinkerConfig();
#else
  config = new (std::nothrow) LinkerConfig(OptTargetTriple);
#endif
  if (config == NULL) {
    llvm::errs() << "Out of memory when create the linker configuration!\n";
    return false;
  }

  Linker::ErrorCode result = pLinker.config(*config);

  delete config;

  if (Linker::kSuccess != result) {
    llvm::errs() << "Failed to configure the linker! (detail: "
                << Linker::GetErrorString(result) << ")\n";
    return false;
  }

  return true;
}

#define DEFAULT_OUTPUT_PATH "a.out"
static inline
std::string DetermineOutputFilename(const std::string pOutputPath)
{
  if (!pOutputPath.empty()) {
    return pOutputPath;
  }

  // User does't specify the value to -o
  if (OptInputObjectFiles.size() > 1) {
    llvm::errs() << "Use " DEFAULT_OUTPUT_PATH " for output file!\n";
    return DEFAULT_OUTPUT_PATH;
  }

  // There's only one input file
  const std::string &input_path = OptInputObjectFiles[0];
  llvm::SmallString<200> output_path(input_path);

  llvm::error_code err = llvm::sys::fs::make_absolute(output_path);
  if (llvm::errc::success != err) {
    llvm::errs() << "Failed to determine the absolute path of `" << input_path
                 << "'! (detail: " << err.message() << ")\n";
    return "";
  }

  llvm::sys::path::remove_filename(output_path);
  llvm::sys::path::append(output_path, "a.out");

  return output_path.c_str();
}

static inline
bool LinkFiles(Linker& pLinker, const std::string &pOutputPath)
{
  return true;
}

int main(int argc, char* argv[])
{
  llvm::cl::SetVersionPrinter(MCLDVersionPrinter);
  llvm::cl::ParseCommandLineOptions(argc, argv);
  init::Initialize();

  Linker linker;
  if (!ConfigLinker(linker)) {
    return EXIT_FAILURE;
  }

  std::string OutputFilename = DetermineOutputFilename(OptOutputFilename);
  if (OutputFilename.empty()) {
    return EXIT_FAILURE;
  }

  if (!LinkFiles(linker, OutputFilename)) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
