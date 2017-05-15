#pragma once

namespace HelloElfLoader {
	// 以下内容复制自
	// https://github.com/aeste/binutils/blob/develop/elfcpp/elfcpp.h
	// https://github.com/aeste/binutils/blob/develop/include/elf/external.h

	// e_ident中各项的偏移值
	const int EI_MAG0 = 0;
	const int EI_MAG1 = 1;
	const int EI_MAG2 = 2;
	const int EI_MAG3 = 3;
	const int EI_CLASS = 4;
	const int EI_DATA = 5;
	const int EI_VERSION = 6;
	const int EI_OSABI = 7;
	const int EI_ABIVERSION = 8;
	const int EI_PAD = 9;
	const int EI_NIDENT = 16;

	// ELF文件类型
	enum {
		ELFCLASSNONE = 0,
		ELFCLASS32 = 1,
		ELFCLASS64 = 2
	};

	// ByteOrder
	enum {
		ELFDATANONE = 0,
		ELFDATA2LSB = 1,
		ELFDATA2MSB = 2
	};

	// 程序头类型
	enum PT
	{
		PT_NULL = 0,
		PT_LOAD = 1,
		PT_DYNAMIC = 2,
		PT_INTERP = 3,
		PT_NOTE = 4,
		PT_SHLIB = 5,
		PT_PHDR = 6,
		PT_TLS = 7,
		PT_LOOS = 0x60000000,
		PT_HIOS = 0x6fffffff,
		PT_LOPROC = 0x70000000,
		PT_HIPROC = 0x7fffffff,
		// The remaining values are not in the standard.
		// Frame unwind information.
		PT_GNU_EH_FRAME = 0x6474e550,
		PT_SUNW_EH_FRAME = 0x6474e550,
		// Stack flags.
		PT_GNU_STACK = 0x6474e551,
		// Read only after relocation.
		PT_GNU_RELRO = 0x6474e552,
		// Platform architecture compatibility information
		PT_ARM_ARCHEXT = 0x70000000,
		// Exception unwind tables
		PT_ARM_EXIDX = 0x70000001
	};

	// 动态节类型
	enum DT
	{
		DT_NULL = 0,
		DT_NEEDED = 1,
		DT_PLTRELSZ = 2,
		DT_PLTGOT = 3,
		DT_HASH = 4,
		DT_STRTAB = 5,
		DT_SYMTAB = 6,
		DT_RELA = 7,
		DT_RELASZ = 8,
		DT_RELAENT = 9,
		DT_STRSZ = 10,
		DT_SYMENT = 11,
		DT_INIT = 12,
		DT_FINI = 13,
		DT_SONAME = 14,
		DT_RPATH = 15,
		DT_SYMBOLIC = 16,
		DT_REL = 17,
		DT_RELSZ = 18,
		DT_RELENT = 19,
		DT_PLTREL = 20,
		DT_DEBUG = 21,
		DT_TEXTREL = 22,
		DT_JMPREL = 23,
		DT_BIND_NOW = 24,
		DT_INIT_ARRAY = 25,
		DT_FINI_ARRAY = 26,
		DT_INIT_ARRAYSZ = 27,
		DT_FINI_ARRAYSZ = 28,
		DT_RUNPATH = 29,
		DT_FLAGS = 30,

		// This is used to mark a range of dynamic tags.  It is not really
		// a tag value.
		DT_ENCODING = 32,

		DT_PREINIT_ARRAY = 32,
		DT_PREINIT_ARRAYSZ = 33,
		DT_LOOS = 0x6000000d,
		DT_HIOS = 0x6ffff000,
		DT_LOPROC = 0x70000000,
		DT_HIPROC = 0x7fffffff,

		// The remaining values are extensions used by GNU or Solaris.
		DT_VALRNGLO = 0x6ffffd00,
		DT_GNU_PRELINKED = 0x6ffffdf5,
		DT_GNU_CONFLICTSZ = 0x6ffffdf6,
		DT_GNU_LIBLISTSZ = 0x6ffffdf7,
		DT_CHECKSUM = 0x6ffffdf8,
		DT_PLTPADSZ = 0x6ffffdf9,
		DT_MOVEENT = 0x6ffffdfa,
		DT_MOVESZ = 0x6ffffdfb,
		DT_FEATURE = 0x6ffffdfc,
		DT_POSFLAG_1 = 0x6ffffdfd,
		DT_SYMINSZ = 0x6ffffdfe,
		DT_SYMINENT = 0x6ffffdff,
		DT_VALRNGHI = 0x6ffffdff,

		DT_ADDRRNGLO = 0x6ffffe00,
		DT_GNU_HASH = 0x6ffffef5,
		DT_TLSDESC_PLT = 0x6ffffef6,
		DT_TLSDESC_GOT = 0x6ffffef7,
		DT_GNU_CONFLICT = 0x6ffffef8,
		DT_GNU_LIBLIST = 0x6ffffef9,
		DT_CONFIG = 0x6ffffefa,
		DT_DEPAUDIT = 0x6ffffefb,
		DT_AUDIT = 0x6ffffefc,
		DT_PLTPAD = 0x6ffffefd,
		DT_MOVETAB = 0x6ffffefe,
		DT_SYMINFO = 0x6ffffeff,
		DT_ADDRRNGHI = 0x6ffffeff,

		DT_RELACOUNT = 0x6ffffff9,
		DT_RELCOUNT = 0x6ffffffa,
		DT_FLAGS_1 = 0x6ffffffb,
		DT_VERDEF = 0x6ffffffc,
		DT_VERDEFNUM = 0x6ffffffd,
		DT_VERNEED = 0x6ffffffe,
		DT_VERNEEDNUM = 0x6fffffff,

		DT_VERSYM = 0x6ffffff0,

		// Specify the value of _GLOBAL_OFFSET_TABLE_.
		DT_PPC_GOT = 0x70000000,

		// Specify the start of the .glink section.
		DT_PPC64_GLINK = 0x70000000,

		// Specify the start and size of the .opd section.
		DT_PPC64_OPD = 0x70000001,
		DT_PPC64_OPDSZ = 0x70000002,

		// The index of an STT_SPARC_REGISTER symbol within the DT_SYMTAB
		// symbol table.  One dynamic entry exists for every STT_SPARC_REGISTER
		// symbol in the symbol table.
		DT_SPARC_REGISTER = 0x70000001,

		DT_AUXILIARY = 0x7ffffffd,
		DT_USED = 0x7ffffffe,
		DT_FILTER = 0x7fffffff
	};;

	// ELF头的定义
	typedef struct {
		unsigned char	e_ident[16];		/* ELF "magic number" */
		unsigned char	e_type[2];		/* Identifies object file type */
		unsigned char	e_machine[2];		/* Specifies required architecture */
		unsigned char	e_version[4];		/* Identifies object file version */
		unsigned char	e_entry[8];		/* Entry point virtual address */
		unsigned char	e_phoff[8];		/* Program header table file offset */
		unsigned char	e_shoff[8];		/* Section header table file offset */
		unsigned char	e_flags[4];		/* Processor-specific flags */
		unsigned char	e_ehsize[2];		/* ELF header size in bytes */
		unsigned char	e_phentsize[2];		/* Program header table entry size */
		unsigned char	e_phnum[2];		/* Program header table entry count */
		unsigned char	e_shentsize[2];		/* Section header table entry size */
		unsigned char	e_shnum[2];		/* Section header table entry count */
		unsigned char	e_shstrndx[2];		/* Section header string table index */
	} Elf64_External_Ehdr;

	// 程序头的定义
	typedef struct {
		unsigned char	p_type[4];		/* Identifies program segment type */
		unsigned char	p_flags[4];		/* Segment flags */
		unsigned char	p_offset[8];		/* Segment file offset */
		unsigned char	p_vaddr[8];		/* Segment virtual address */
		unsigned char	p_paddr[8];		/* Segment physical address */
		unsigned char	p_filesz[8];		/* Segment size in file */
		unsigned char	p_memsz[8];		/* Segment size in memory */
		unsigned char	p_align[8];		/* Segment alignment, file & memory */
	} Elf64_External_Phdr;

	// DYNAMIC类型的程序头的内容定义
	typedef struct {
		unsigned char	d_tag[8];		/* entry tag value */
		union {
			unsigned char	d_val[8];
			unsigned char	d_ptr[8];
		} d_un;
	} Elf64_External_Dyn;

	// 动态链接的重定位记录，部分系统会用Elf64_External_Rel
	typedef struct {
		unsigned char r_offset[8];	/* Location at which to apply the action */
		unsigned char	r_info[8];	/* index and type of relocation */
		unsigned char	r_addend[8];	/* Constant addend used to compute value */
	} Elf64_External_Rela;

	// 动态链接的符号信息
	typedef struct {
		unsigned char	st_name[4];		/* Symbol name, index in string tbl */
		unsigned char	st_info[1];		/* Type and binding attributes */
		unsigned char	st_other[1];		/* No defined meaning, 0 */
		unsigned char	st_shndx[2];		/* Associated section index */
		unsigned char	st_value[8];		/* Value of the symbol */
		unsigned char	st_size[8];		/* Associated symbol size */
	} Elf64_External_Sym;
}
