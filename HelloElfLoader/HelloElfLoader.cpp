#include "stdafx.h"
#include "ElfDefine.h"
#include "LibraryFuncs.h"
#include "HelloElfLoader.h"
#include <Windows.h>
#include <iostream>
#include <vector>
#include <memory>

namespace HelloElfLoader {
	Loader::Loader(const std::string& path) :
		Loader(std::ifstream(path, std::ios::in | std::ios::binary)) {}

	Loader::Loader(std::ifstream&& fileStream) :
		fileStream_(std::move(fileStream)) {
		if (!fileStream_) {
			throw std::runtime_error("open file failed");
		}
	}

	void Loader::execute() {
		std::cout << "====== start loading elf ======" << std::endl;

		// 检查当前运行程序是否64位
		if (sizeof(intptr_t) != sizeof(std::int64_t)) {
			throw std::runtime_error("please use x64 compile and run this program");
		}

		// 读取ELF头
		Elf64_External_Ehdr elfHeader = {};
		fileStream_.seekg(0);
		fileStream_.read(reinterpret_cast<char*>(&elfHeader), sizeof(elfHeader));

		// 检查ELF头，只支持64位且byte order是little endian的程序
		if (std::string(reinterpret_cast<char*>(elfHeader.e_ident), 4) != "\x7f\x45\x4c\x46") {
			throw std::runtime_error("magic not match");
		}
		else if (elfHeader.e_ident[EI_CLASS] != ELFCLASS64) {
			throw std::runtime_error("only support ELF64");
		}
		else if (elfHeader.e_ident[EI_DATA] != ELFDATA2LSB) {
			throw std::runtime_error("only support little endian");
		}

		// 获取program table的信息
		std::uint32_t programTableOffset = *reinterpret_cast<std::uint32_t*>(elfHeader.e_phoff);
		std::uint16_t programTableEntrySize = *reinterpret_cast<std::uint16_t*>(elfHeader.e_phentsize);
		std::uint16_t programTableEntryNum = *reinterpret_cast<std::uint16_t*>(elfHeader.e_phnum);
		std::cout << "program table at: " << programTableOffset << ", "
			<< programTableEntryNum << " x " << programTableEntrySize << std::endl;

		// 获取section table的信息
		// section table只给linker用，loader中其实不需要访问section table
		std::uint32_t sectionTableOffset = *reinterpret_cast<std::uint32_t*>(elfHeader.e_shoff);
		std::uint16_t sectionTableEntrySize = *reinterpret_cast<std::uint16_t*>(elfHeader.e_shentsize);
		std::uint16_t sectionTableEntryNum = *reinterpret_cast<std::uint16_t*>(elfHeader.e_shentsize);
		std::cout << "section table at: " << sectionTableOffset << ", "
			<< sectionTableEntryNum << " x " << sectionTableEntrySize << std::endl;

		// 准备动态链接的信息
		std::uint64_t jmpRelAddr = 0; // 重定位记录的开始地址
		std::uint64_t pltRelType = 0; // 重定位记录的类型 RELA或REL
		std::uint64_t pltRelSize = 0; // 重定位记录的总大小
		std::uint64_t symTabAddr = 0; // 动态符号表的开始地址
		std::uint64_t strTabAddr = 0; // 动态符号名称表的开始地址
		std::uint64_t strTabSize = 0; // 动态符号名称表的总大小

		// 遍历program hedaer
		std::vector<Elf64_External_Phdr> programHeaders;
		programHeaders.resize(programTableEntryNum);
		fileStream_.read(reinterpret_cast<char*>(programHeaders.data()), programTableEntryNum * programTableEntrySize);
		std::vector<std::shared_ptr<void>> loadedSegments;
		for (const auto& programHeader : programHeaders) {
			std::uint32_t type = *reinterpret_cast<const std::uint32_t*>(programHeader.p_type);
			if (type == PT_LOAD) {
				// 把文件内容(包含程序代码和数据)加载到虚拟内存，这个示例不考虑地址冲突
				std::uint64_t fileOffset = *reinterpret_cast<const std::uint64_t*>(programHeader.p_offset);
				std::uint64_t fileSize = *reinterpret_cast<const std::uint64_t*>(programHeader.p_filesz);
				std::uint64_t virtAddr = *reinterpret_cast<const std::uint64_t*>(programHeader.p_vaddr);
				std::uint64_t memSize = *reinterpret_cast<const std::uint64_t*>(programHeader.p_memsz);
				if (memSize < fileSize) {
					throw std::runtime_error("invalid memsz in program header, it shouldn't less than filesz");
				}
				// 在指定的虚拟地址分配内存
				std::cout << std::hex << "allocate address at: 0x" << virtAddr <<
					" size: 0x" << memSize << std::dec << std::endl;
				void* addr = ::VirtualAlloc((void*)virtAddr, memSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
				if (addr == nullptr) {
					throw std::runtime_error("allocate memory at specific address failed");
				}
				loadedSegments.emplace_back(addr, [](void* ptr) { ::VirtualFree(ptr, 0, MEM_RELEASE); });
				// 复制文件内容到虚拟内存
				fileStream_.seekg(fileOffset);
				if (!fileStream_.read(reinterpret_cast<char*>(addr), fileSize)) {
					throw std::runtime_error("read contents into memory from LOAD program header failed");
				}
			}
			else if (type == PT_DYNAMIC) {
				// 遍历动态节
				std::uint64_t fileOffset = *reinterpret_cast<const std::uint64_t*>(programHeader.p_offset);
				fileStream_.seekg(fileOffset);
				Elf64_External_Dyn dynSection = {};
				std::uint64_t dynSectionTag = 0;
				std::uint64_t dynSectionVal = 0;
				do {
					if (!fileStream_.read(reinterpret_cast<char*>(&dynSection), sizeof(dynSection))) {
						throw std::runtime_error("read dynamic section failed");
					}
					dynSectionTag = *reinterpret_cast<const std::uint64_t*>(dynSection.d_tag);
					dynSectionVal = *reinterpret_cast<const std::uint64_t*>(dynSection.d_un.d_val);
					if (dynSectionTag == DT_JMPREL) {
						jmpRelAddr = dynSectionVal;
					}
					else if (dynSectionTag == DT_PLTREL) {
						pltRelType = dynSectionVal;
					}
					else if (dynSectionTag == DT_PLTRELSZ) {
						pltRelSize = dynSectionVal;
					}
					else if (dynSectionTag == DT_SYMTAB) {
						symTabAddr = dynSectionVal;
					}
					else if (dynSectionTag == DT_STRTAB) {
						strTabAddr = dynSectionVal;
					}
					else if (dynSectionTag == DT_STRSZ) {
						strTabSize = dynSectionVal;
					}
				} while (dynSectionTag != 0);
			}
		}

		// 读取动态链接符号表
		std::string dynamicSymbolNames(reinterpret_cast<char*>(strTabAddr), strTabSize);
		Elf64_External_Sym* dynamicSymbols = reinterpret_cast<Elf64_External_Sym*>(symTabAddr);

		// 设置动态链接的函数地址
		std::cout << std::hex << "read dynamic entires at: 0x" << jmpRelAddr <<
			" size: 0x" << pltRelSize << std::dec << std::endl;
		if (jmpRelAddr == 0 || pltRelType != DT_RELA || pltRelSize % sizeof(Elf64_External_Rela) != 0) {
			throw std::runtime_error("invalid dynamic entry info, rel type should be rela");
		}
		std::vector<std::shared_ptr<void>> libraryFuncs;
		for (std::uint64_t offset = 0; offset < pltRelSize; offset += sizeof(Elf64_External_Rela)) {
			Elf64_External_Rela* rela = (Elf64_External_Rela*)(jmpRelAddr + offset);
			std::uint64_t relaOffset = *reinterpret_cast<const std::uint64_t*>(rela->r_offset);
			std::uint64_t relaInfo = *reinterpret_cast<const std::uint64_t*>(rela->r_info);
			std::uint64_t relaSym = relaInfo >> 32; // ELF64_R_SYM
			std::uint64_t relaType = relaInfo & 0xffffffff; // ELF64_R_TYPE
			// 获取符号
			Elf64_External_Sym* symbol = dynamicSymbols + relaSym;
			std::uint32_t symbolNameOffset = *reinterpret_cast<std::uint32_t*>(symbol->st_name);
			std::string symbolName(dynamicSymbolNames.data() + symbolNameOffset);
			std::cout << "relocate symbol: " << symbolName << std::endl;
			// 替换函数地址
			// 原本应该延迟解决，这里图简单就直接覆盖了
			void** relaPtr = reinterpret_cast<void**>(relaOffset);
			std::shared_ptr<void> func = resolveLibraryFunc(symbolName);
			if (func == nullptr) {
				throw std::runtime_error("unsupport symbol name");
			}
			libraryFuncs.emplace_back(func);
			*relaPtr = func.get();
		}

		// 获取入口点
		std::uint64_t entryPointAddress = *reinterpret_cast<const std::uint64_t*>(elfHeader.e_entry);
		void(*entryPointFunc)() = reinterpret_cast<void(*)()>(entryPointAddress);
		std::cout << "entry point: " << entryPointFunc << std::endl;
		std::cout << "====== finish loading elf ======" << std::endl;

		// 执行主程序
		// 会先调用__libc_start_main, 然后再调用main
		// 调用__libc_start_main后的指令是hlt，所以必须在__libc_start_main中退出执行
		entryPointFunc();
	}
}
