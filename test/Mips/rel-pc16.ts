# Check handling of R_MIPS_PC16 relocation.

# RUN: yaml2obj -format=elf %s > %t.o
# RUN: %MCLinker -mtriple=mipsel-unknown-linux -e T0 -o %t.exe %t.o
# RUN: llvm-objdump -s -t %t.exe | FileCheck %s

# CHECK: Contents of section .text:
# CHECK-NEXT: {{[0-9A-F]+}} feff0000 00000000 00000000
#                           ^ V
#                             A = -16 =>
#                             V = (T1 - 16 - T0) >> 2 = -2

# CHECK: SYMBOL TABLE:
# CHECK: {{[0-9A-F]+}} g  F .text  00000008 T0
# CHECK: {{[0-9A-F]+}} g  F .text  00000004 T1

FileHeader:
  Class:   ELFCLASS32
  Data:    ELFDATA2LSB
  Type:    ET_REL
  Machine: EM_MIPS
  Flags:   [EF_MIPS_CPIC, EF_MIPS_ABI_O32, EF_MIPS_ARCH_32]

Sections:
- Name:         .text
  Type:         SHT_PROGBITS
  Content:      "fcff00000000000000000000"
#                                ^ T1
#                ^ T0 A := 0xfffc << 2 = -16
  AddressAlign: 16
  Flags:        [ SHF_ALLOC, SHF_EXECINSTR ]

- Name:         .rel.text
  Type:         SHT_REL
  Info:         .text
  AddressAlign: 4
  Relocations:
    - Offset: 0
      Symbol: T1
      Type:   R_MIPS_PC16

Symbols:
  Global:
    - Name:    T0
      Section: .text
      Type:    STT_FUNC
      Value:   0
      Size:    8
    - Name:    T1
      Section: .text
      Type:    STT_FUNC
      Value:   8
      Size:    4
