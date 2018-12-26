
./build_access.py raw_elf_definesdwarf_reloc_mips.h mips dwarf_elf_reloc
./build_access.py raw_elf_definesdwarf_reloc_386.h 386 dwarf_elf_reloc

./build_access.py raw_elf_definesdwarf_reloc_arm.h arm dwarf_elf_reloc

./build_access.py raw_elf_definesdwarf_reloc_ppc64.h ppc64 dwarf_elf_reloc

./build_access.py raw_elf_definesdwarf_reloc_ppc.h ppc dwarf_elf_reloc

./build_access.py raw_elf_definesdwarf_reloc_x86_64.h x86_64 dwarf_elf_reloc

#cc -c dwarf_elf_reloc_386.c 
#cc -c dwarf_elf_reloc_arm.c
#cc -c dwarf_elf_reloc_mips.c
#cc -c dwarf_elf_reloc_ppc.c 
#cc -c dwarf_elf_reloc_ppc64.c 
#cc -c dwarf_elf_reloc_x86_64.c 

cp dwarf_elf_reloc_386.c \
	dwarf_elf_reloc_386.h \
	dwarf_elf_reloc_arm.c \
	dwarf_elf_reloc_arm.h \
	dwarf_elf_reloc_mips.c \
	dwarf_elf_reloc_mips.h \
	dwarf_elf_reloc_ppc.c \
	dwarf_elf_reloc_ppc.h \
	dwarf_elf_reloc_ppc64.c \
	dwarf_elf_reloc_ppc64.h \
	dwarf_elf_reloc_x86_64.c \
	dwarf_elf_reloc_x86_64.h  ../src



